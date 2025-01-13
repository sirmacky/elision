#include "TestRunner.h"

#include "TestRunner.h"

#include "TestResult.h"

#include <thread>
#include <vector>
#include <latch>
#include <functional>
#include <iostream>
#include <chrono>
#include <memory>
#include <future>

#if defined _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#undef min
#undef max
namespace ThreadUtils
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

//===========================================================================================================
void TestContext::SetFailure(const std::string& reason)
{
	SetFailure(test_failure(reason, Definition->_file, Definition->_lineNumber));
}

void TestContext::SetFailure(const test_failure& reason)
{
	Result->SetFailure(reason);
}

std::chrono::milliseconds TestContext::DetermineTimeout(const ExecutionOptions& options) const
{
	using namespace std::chrono_literals;

	auto timeout = Definition->_timeout;
	if (timeout <= 0ms)
		timeout = options.DefaultTimeOut;

	if (options.MaximumTimeout.has_value())
		timeout = std::min(timeout, options.MaximumTimeout.value());

	return timeout;
}
//===========================================================================================================
void TestRunner::Run(std::unordered_set<const TestDefinition*> tests, const ExecutionOptions& options)
{
	if (Status != Status::Idle)
	{
		Cancel();
		if (Status != Status::Idle)
			return;
	}

	// Determine if we need to cancel first.
	Status = Status::Running;
	
	// if there are no threads, then execute everything on the main thread
	if (options.MaxNumberOfSimultaneousThreads == 0)
	{
		for (const auto* test : tests)
			TestRunner::Run(test, options);
		return;
	}

	_stopSource = {};
	_thread = std::thread([=, this]()
	{
		Run(tests, options, _stopSource.get_token());
		this->Status = Status::Done;
	});
}
	
void TestRunner::Run(std::unordered_set<const TestDefinition*> tests, const ExecutionOptions& options, std::stop_token token)
{
	// Split the tests into different cohorts
	std::array<std::vector<const TestDefinition*>, static_cast<int>(TestConcurrency::Count)> _cohorts;
	for (const auto* test : tests)
	{
		auto concurrency = test->_concurrency;

		concurrency = std::min(concurrency, options.MaximumConcurrency.value_or(concurrency));
		concurrency = options.EnforcedConcurrency.value_or(concurrency);
		_cohorts[static_cast<int>(concurrency)].push_back(test);
	}

	// anything that is exclusive we run now.
	TestRunner::RunAsync(std::span(_cohorts[static_cast<int>(TestConcurrency::Exclusive)]), options, token);

	if (token.stop_requested())
		return;

	// Create worker threads for our remainder, and allow them to take from the Any pool
	// we will maintain as our own worker thread and process the Privelaged, before assisting with the remaining pool
	const auto& remainder = _cohorts[static_cast<int>(TestConcurrency::Any)];
	const auto& privelaged = _cohorts[static_cast<int>(TestConcurrency::Privelaged)];

	int numAdditionalThreads = 0;
	// determine how many additional threads will be needed 
	if (remainder.size() > 0)
	{
		int totalRemainingTests = (int)(remainder.size() + privelaged.size());
		int preferredNumThreads = (int)(totalRemainingTests / options.MinimumNumberOfTestsPerThread);

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

			const auto* definition = remainder[index];

			TestRunner::Run(definition, options);
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

void TestRunner::Cancel()
{
	using namespace std::chrono_literals;

	if (Status != Status::Running)
		return;

	// send a stop token
	Status = Status::Cancelling;
	_stopSource.request_stop();
}

void TestRunner::Join()
{
	if (_thread.joinable())
		_thread.join();
}

void TestRunner::RunAsync(std::span<const TestDefinition* const> tests, const ExecutionOptions& options, std::stop_token token)
{
	for (const auto* test : tests)
	{
		if (token.stop_requested())
			return;

		TestRunner::Run(test, options);
	}
}

void TestRunner::Run(const TestDefinition* const definition, const ExecutionOptions& options)
{
	TestResult result;
	TestRunner::Run({definition, &result }, options);
}

void TestRunner::Run(TestContext context, const ExecutionOptions& options)
{
	using namespace std::chrono_literals;
	
	auto timeout = context.DetermineTimeout(options);

	std::atomic<bool> complete{ false };
	std::thread thr([&]()	{
		TestRunner::RunInternal(context, options);
		complete = true;
	});

	auto startTime = std::chrono::high_resolution_clock::now().time_since_epoch();
	while (!complete)
	{
		if (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime >= timeout)
		{
			context.SetFailure(std::format("exceeded timeout duration of {}", timeout));
			ThreadUtils::KillThread(thr);
			break;
		}
	}
};

void TestRunner::RunInternal(TestContext context, const ExecutionOptions& options)
{
	context.Result->Reset();

	try
	{
		context.Result->Begin(std::chrono::high_resolution_clock::now().time_since_epoch());
		std::invoke(context.Definition->_test);
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
	
	context.Result->End(std::chrono::high_resolution_clock::now().time_since_epoch());

	if (auto timeout = context.DetermineTimeout(options); context.Result->TimeTaken() > timeout)
	{
		context.SetFailure(std::format("exceeded timeout duration of {}", timeout));
	}
}

