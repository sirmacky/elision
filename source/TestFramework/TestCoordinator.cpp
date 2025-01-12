#include "TestCoordinator.h"

#include "TestRunner.h"

#include <thread>
#include <vector>
#include <latch>
#include <functional>
#include <iostream>
#include <chrono>
#include <memory>
#include <future>

/*
#if defined(_WIN32)
	#include <windows.h>
	#include <processthreadsapi.h>
	#undef min // so fucking annoying
#endif

namespace ThreadUtils
{
	
	void KillThread(std::jthread& thread)
	{
#if defined(_WIN32)
		TerminateThread(thread.native_handle(), 0);
		thread.detach();
#endif
	}

	void KillThread(std::thread& thread)
	{
#if defined(_WIN32)
		TerminateThread(thread.native_handle(), 0);
		thread.detach();
#endif
	}
	
}
*/

class ThreadManager 
{
public:
	// Start the manager in its own thread and return a future
	static std::future<void> launch( int N, std::function<void()> worker_task, std::function<void()> final_task) 
	{
		return std::async(std::launch::async, [=]()
		{
			run(N, worker_task, final_task);
		});
	}

	// Can also be used synchronously if needed
	static void run( int N, std::function<void()> worker_task, std::function<void()> final_task) 
	{
		ThreadManager manager(N);
		manager.execute(worker_task, final_task);
	}

private:
	ThreadManager(int N) : N(N), count_down(N) {}

	void execute(const std::function<void()>& worker_task, const std::function<void()>& final_task) 
	{
		std::vector<std::jthread> workers;
		workers.reserve(N);

		// Launch N worker threads
		for (int i = 0; i < N; i++) 
		{
			workers.emplace_back([this, &worker_task]() 
			{
				worker_task();
				count_down.count_down();
			});
		}

		// Launch final thread that waits for workers
		workers.emplace_back([this, &final_task]() 
		{
			count_down.wait();
			final_task();
		});

		// Join all threads
		for (auto& worker : workers) {
			worker.join();
		}
	}

	const int N;
	std::latch count_down;
};

TestCoordinator::TestCoordinator(std::vector<const TestDefinition*> tests, const ExecutionOptions& options)
{
	Status = Status::Running;
	if (options.MaxNumberOfSimultaneousThreads == 0) // if there are no threads, then execute everything on the main thread
	{
		for (const auto* test : tests)
			RunTest(test);
		return;
	}

	if (options.MaxNumberOfSimultaneousThreads == 1)
	{
		RunAsync(std::span(tests));
		return;
	}

	std::vector<std::span<const TestDefinition* const>> work_units;

	// Split the tests into different cohorts
	for (const auto* test : tests)
	{
		auto concurrency = test->_concurrency;
		
		concurrency = std::min(concurrency, options.MaximumConcurrency.value_or(concurrency));
		concurrency = options.EnforcedConcurrency.value_or(concurrency);
		_cohorts[static_cast<int>(concurrency)].push_back(test);
	}

	// create a thread for the privelaged
	// these can conflict with one another
	const auto& privelaged = _cohorts[static_cast<int>(TestConcurrency::Privelaged)];
	if (privelaged.size() > 0)
	{
		// TODO: create a thread and increment the consumed thread count
		work_units.push_back(std::span(privelaged));
	}

	// determine buckets for the remainder
	// these can be chunked into whatever needs to be there
	const auto& remainder = _cohorts[static_cast<int>(TestConcurrency::Any)];
	if (remainder.size() > 0)
	{
		float requiredBuckets = (float)remainder.size() / (options.MinimumNumberOfTestsPerThread);

		int remainingThreads = options.MaxNumberOfSimultaneousThreads - static_cast<int>(work_units.size());

		int numThreads = remainingThreads;
		
		// split them into sub ranges of our cohort and create a thread for each one
		int testsPerThread = (remainder.size() / numThreads);
		int numRemaining = remainder.size() % numThreads;
		
		int consumedTests = 0;
		for (int i = 0; i < numThreads; ++i)
		{
			int numTests = testsPerThread;
			if (i < numRemaining)
				numTests += 1;

			auto start = (remainder.begin() + consumedTests);
			consumedTests += numTests;

			work_units.emplace_back(start, numTests);
		}
	}

	ExclusiveLatch = std::latch(work_units.size());

	for (auto& work_unit : work_units)
	{
		RunAsyncThenDecrement(work_unit, ExclusiveLatch);
	}

	
	// TODO: once the other threads have completed, then we can run exclusive
	auto& exclusives = _cohorts[static_cast<int>(TestConcurrency::Exclusive)];
	if (exclusives.size() > 0)
	{
		RunAsyncWhenReady(std::span(exclusives), ExclusiveLatch);
	}
}

void TestCoordinator::Cancel()
{
	using namespace std::chrono_literals;

	if (Status != Status::Running)
		return;

	// send a stop token
	Status = Status::Cancelling;

	for (auto& thread : _threads)
	{
		thread.request_stop();
	}
		
	// wait 100 ms
	std::this_thread::sleep_for(100ms);

	// annihlate the remaining tests.
	 
	// if the tests have not ceased, then wait 1 second
	
	/* This shouldnt be necessary as the thread destructor will abort the thread for us
	// if the tests have not ceased, force close them. leak the memory, fuck it.
	for (auto& thread : _threads)
		ThreadUtils::KillThread(thread);
	*/
	_threads.clear();

	// TODO: Cleanup
}

void TestCoordinator::RunAsyncWhenReady(std::span<const TestDefinition* const> tests, std::latch& latch)
{
	_threads.emplace_back([&latch, tests](std::stop_token token)
	{
		while (latch.try_wait())
		{
			if (token.stop_requested())
				return;
		}
		
		RunTests(tests, token);
	});
}

void TestCoordinator::RunAsyncThenDecrement(std::span<const TestDefinition* const> tests, std::latch& latch)
{
	_threads.emplace_back([&latch, tests](std::stop_token token)
	{
		RunTests(tests, token);

		if (token.stop_requested())
			return;

		latch.count_down();
	});
}

void TestCoordinator::RunAsync(std::span<const TestDefinition* const> tests)
{
	_threads.emplace_back([tests](std::stop_token token)
	{
		RunTests(tests, token);
	});
}

void TestCoordinator::RunTests(std::span<const TestDefinition* const> tests, std::stop_token token)
{
	for (const auto* test : tests)
	{
		if (token.stop_requested())
			return;

		RunTest(test);
	}
}

void TestCoordinator::RunTest(const TestDefinition* definition)
{
	TestResult result;
	TestContext context{ };
	TestRunner::Run(context);

	// TODO: publish the result;
	PublishResult(context);
}

void TestCoordinator::PublishResult(const TestContext& context)
{

}

//===========================================================================================================

void TestContext::SetFailure(const std::string& reason)
{
	SetFailure(test_failed(reason, Definition->_file, Definition->_lineNumber));
}

void TestContext::SetFailure(const test_failed& reason)
{
	Result->SetFailure(reason);
}

std::chrono::milliseconds TestContext::DetermineTimeout() const
{
	using namespace std::chrono_literals;

	auto timeout = Definition->_timeout;
	if (timeout <= 0ms)
	{
		timeout = Options->DefaultTimeOut;
	}

	if (Options->MaximumTimeout.has_value())
	{
		timeout = std::min(timeout, Options->MaximumTimeout.value());
	}

	return timeout;
}

struct ExecutionContext
{
	std::array<std::vector<const TestDefinition*>, static_cast<int>(TestConcurrency::Count)> _cohorts;
	TestDefinition* Pull(bool allowElevated)
	{
		
	}
};


TestRunner::TestRunner(std::vector<const TestDefinition*> tests, const ExecutionOptions& options = ExecutionOptions())
{
	// determine how many threads we NEED versus how many we can have

	// create the threads.

	// Generate an execution context for them to sychronize with that can detect when the elevated thread is allowed to take extra

	// So we have one elevated thread that runs
	// (privelaged, any, exclusive) in that order
	// then we have unprivelaged threads that can only run concurrncy::any and take from the bucket as they go
	// this avoids the latch as we no longer need it
	ExecutionContext context;
	std::vector<std::jthread> _threads;
	int numThreads = 0;
	for (int i = 0; i < numThreads; ++i)
	{
		bool elevated = i == 0;
		_threads.emplace_back([elevated, tests, ctr = &context] (std::stop_token token) 
		{


		});
	}

	std::stop_token token;
	auto t = [tests, options, token]()
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
		TestRunner::Run(std::span(_cohorts[static_cast<int>(TestConcurrency::Exclusive)]), options, token);

		if (token.stop_requested())
			return;

		// Create worker threads for our remainder, and allow them to take from the Any pool
		// we will maintain as our own worker thread and process the Privelaged, before assisting with the remaining pool

		std::vector<std::thread> threads;
		std::atomic<size_t> work_index{ 0 };

		const auto& remainder = _cohorts[static_cast<int>(TestConcurrency::Any)];
		auto pool_worker = [&]()
		{
			while (!token.stop_requested())
			{
				size_t index = work_index++;
				if (index >= remainder.size())
					break;

				const auto* definition = remainder[index];

				const ExecutionOptions& options
				TestRunner::RunAsync()
				// TODO:
				
			}
		};
		
		const auto& privelaged = _cohorts[static_cast<int>(TestConcurrency::Privelaged)];
		if (remainder.size() > 0)
		{
			int totalRemainingTests = (int)(remainder.size() + privelaged.size());

			int requiredBuckets = std::max(totalRemainingTests / (options.MinimumNumberOfTestsPerThread), 1);
			int remainingThreads = options.MaxNumberOfSimultaneousThreads - 1;
			int numThreads = std::min(requiredBuckets, remainingThreads);

			// kick these threads off.
			while (numThreads--)
			{
				threads.emplace_back(pool_worker);
			}
		}

		// Run the privelaged
		TestRunner::Run(std::span(privelaged), options, token);

		// Help with the remainder of the any tests
		std::invoke(pool_worker);

		// Join the rest of the threads
		for (auto& thread : threads)
			thread.join();

		// and we're done!
	};
}

bool TestRunner::TryRunNextTest(std::atomic<size_t>& workIndex, std::vector<const TestDefinition*>& work_pool)
{
	// what if we keep an atomic int for the next index, this would mean we don't need to do any weird operations
	// it only needs to be kept by the controller thread too, 
	size_t index = workIndex++;
	if (index >= work_pool.size())
		return false;

	const auto* definition = work_pool[index];
	return true;
}

void TestRunner::Run(std::span<const TestDefinition* const> tests, const ExecutionOptions& options, std::stop_token token)
{
	for (const auto* test : tests)
	{
		if (token.stop_requested())
			return;

		TestResult result;
		TestContext context{ &options, test, &result };
		TestRunner::Run(context);
	}
}

void TestCoordinator::RunTest(const TestDefinition* definition)
{
	// TODO: publish the result;
}

void TestRunner::Run(TestContext context)
{
	context.Result->Reset();

	try
	{
		context.Result->Begin(std::chrono::high_resolution_clock::now().time_since_epoch());
		std::invoke(context.Definition->_test);
	}
	catch (test_failed failure)
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
	
}

void TestRunner::RunAsync(TestContext context)
{
	// This is a single run of the test
	std::thread thr([context]() { TestRunner::Run(context); });
	auto future = std::async(std::launch::async, &std::thread::join, &thr);

	// todo: the context should have the timeout
	auto timeout = context.DetermineTimeout();
	// if we're cancelled then this will execute fairly quiickly if possible.
	if (future.wait_for(timeout) == std::future_status::timeout)
	{
		context.SetFailure(std::format("exceeded timeout duration of {}", timeout));
	}

	// the thread's destructor should kill the process?
	
};