#include "TestRunner.h"

#include "TestResult.h"
#include "TestObject.h"
#include "TestDefinition.h"

#include <thread>
#include <vector>
#include <latch>
#include <functional>
#include <iostream>
#include <chrono>
#include <memory>
#include <future>

#if defined _WIN32
#define NOMINMAX
#include <Windows.h>
#include <processthreadsapi.h>

namespace lsn::thread_utils
{
	bool KillThread(std::thread& thread)
	{
		if (!thread.joinable())
			return false;

		auto handle = thread.native_handle();
		thread.detach();
		bool ret = TerminateThread(handle, 0);
		
		return ret;
	}
}

#endif

namespace lsn::test_framework
{

//===========================================================================================================
void TestContext::SetFailure(const std::string& reason)
{
	SetFailure(test_failure(reason, Definition->_parent->File, Definition->_parent->LineNumber));
}

void TestContext::SetFailure(const test_failure& reason)
{
	Result->SetFailure(reason);
	Result->End(std::chrono::high_resolution_clock::now().time_since_epoch());
}

std::chrono::milliseconds TestContext::DetermineTimeout(const TestExecutionOptions& options) const
{
	using namespace std::chrono_literals;

	auto timeout = Definition->Timeout;
	if (timeout <= 0ms)
		timeout = options.DefaultTimeOut;

	if (options.MaximumTimeout.has_value())
		timeout = std::min(timeout, options.MaximumTimeout.value());

	return timeout;
}
//===========================================================================================================

TestRunner::~TestRunner()
{
	if (_thread.joinable())
		lsn::thread_utils::KillThread(_thread);
}

bool TestRunner::IsScheduled(const TestDefinition* test) const {
	return _tests.contains(test);
}


void TestRunner::Run(std::vector<TestContext>& tests, const TestExecutionOptions& options)
{
	if (Status != Status::Idle)
	{
		Cancel();
		if (Status != Status::Idle)
			return;
	}

	// Determine if we need to cancel first.
	Status = Status::Running;
	_tests.clear();
	for (const auto& context : tests)
		_tests.insert(context.Definition);
	
	_stopSource = {};

	// if there are no threads, then execute everything on the main thread
	if (options.MaxNumberOfSimultaneousThreads == 0)
	{
		TestRunner::RunAll(tests, options, this->_stopSource.get_token());
		return;
	}

	if (_thread.joinable())
		_thread.join();
	
	_thread = std::thread([=, this]() mutable
	{
		TestRunner::RunAll(tests, options, this->_stopSource.get_token());
		this->OnFinish();
	});
}

void TestRunner::Cancel()
{
	using namespace std::chrono_literals;

	if (Status != Status::Running)
		return;

	// send a stop token
	Status = Status::Cancelling;
	_stopSource.request_stop();

	Join();
}

void TestRunner::Join()
{
	if (_thread.joinable())
		_thread.join();
}

void TestRunner::OnFinish()
{
	Status = Status::Idle;
	_tests.clear();
}
	
void TestRunner::RunAll(std::vector<TestContext>& tests, const TestExecutionOptions& options, std::stop_token token)
{
	// Split the tests into different cohorts
	std::array<std::vector<TestContext*>, static_cast<int>(TestConcurrency::Count)> _cohorts;
	for (auto& context : tests)
	{
		auto concurrency = context.Definition->Concurrency;

		concurrency = std::min(concurrency, options.MaximumConcurrency.value_or(concurrency));
		concurrency = options.EnforcedConcurrency.value_or(concurrency);
		_cohorts[static_cast<int>(concurrency)].push_back(&context);
	}

	// anything that is exclusive we run now.
	TestRunner::RunAsync(std::span(_cohorts[static_cast<int>(TestConcurrency::Exclusive)]), options, token);

	if (token.stop_requested())
		return;

	// Create worker threads for our remainder, and allow them to take from the Any pool
	// we will maintain as our own worker thread and process the Privileged, before assisting with the remaining pool
	const auto& remainder = _cohorts[static_cast<int>(TestConcurrency::Any)];
	const auto& privelaged = _cohorts[static_cast<int>(TestConcurrency::Privileged)];

	int numAdditionalThreads = 0;
	// determine how many additional threads will be needed 
	if (remainder.size() > 0)
	{
		int totalRemainingTests = (int)(remainder.size() + privelaged.size());
		int	preferredNumThreads = (int)(totalRemainingTests / std::max(options.MinimumNumberOfTestsPerThread, 1));
		

		int availableThreads = options.MaxNumberOfSimultaneousThreads - 1;
		numAdditionalThreads = std::min(preferredNumThreads, availableThreads);
	}

	std::atomic<size_t> work_index{ 0 };
	auto pool_worker = [&]()
	{
		while (!token.stop_requested())
		{
			size_t index = work_index++;
			if (index >= remainder.size())
				break;

			TestRunner::Run(*remainder[index], options, token);
		}
	};

	// Generate threads as needed
	std::vector<std::thread> threads;
	while (numAdditionalThreads-- > 0)
		threads.emplace_back(pool_worker);

	// Run the privelaged on our thread
	TestRunner::RunAsync(std::span(privelaged), options, token);

	// Help with the remainder of the any tests
	std::invoke(pool_worker);

	// Join the rest of the threads
	for (auto& thread : threads)
		thread.join();

	// and we're done!
}

void TestRunner::RunAsync(std::span<TestContext* const> tests, const TestExecutionOptions& options, std::stop_token token)
{
	for (auto* test : tests)
	{
		if (token.stop_requested())
			return;

		TestRunner::Run(*test, options, token);
	}
}

// Intentional copy of the context
void TestRunner::Run(TestContext context, const TestExecutionOptions& options, std::stop_token token)
{
	using namespace std::chrono_literals;

	auto timeout = context.DetermineTimeout(options);

	// TODO: This has a crash issue as the context can be destroyed while the thread is being killed at the assignment stage.
	// Need a static synchronization system that will allow these to communicate better. (id's in a set maybe?)
	// potentially a second stop token?
	
	std::atomic<bool> complete{ false };
	std::thread thr([c=context, o=options, &complete]() mutable {
		
		TestRunner::RunInternal(c, o);
		complete = true;
		
	});

	auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();
	while (!complete)
	{
		auto delta = std::chrono::high_resolution_clock::now().time_since_epoch() - startTime;
		if (delta >= timeout)
		{
			lsn::thread_utils::KillThread(thr);
			context.SetFailure(std::format("exceeded timeout duration of {}", timeout));
			break;
		}
		
		if (token.stop_requested())
		{
			// TODO: This causing problems currently.
			// lsn::thread_utils::KillThread(thr);
			context.SetFailure(std::format("cancelled"));
			break;
		}
	}

	if (thr.joinable())
		thr.join();
};

void TestRunner::RunInternal(TestContext& context, const TestExecutionOptions& options)
{
	context.Result->Reset();

	try
	{
		context.Result->Begin(std::chrono::high_resolution_clock::now().time_since_epoch());
		std::invoke(context.Definition->_test);
		context.Result->End(std::chrono::high_resolution_clock::now().time_since_epoch());
	}
	catch (test_failure failure)
	{
		context.SetFailure(failure);
		// TODO:
	}
	catch (std::exception unexpected_failure)
	{
		context.SetFailure(unexpected_failure.what());
	}
	catch (...) // unknown failure
	{
		context.SetFailure("uknown exception encountered");
	}

	if (auto timeout = context.DetermineTimeout(options); context.Result->TimeTaken() > timeout)
	{
		context.SetFailure(std::format("exceeded timeout duration of {}", timeout));
	}
}

}