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

void TestCoordinator::Run(std::vector<const TestDefinition*> tests)
{
	if (MaxNumberOfSimultaneousThreads == 0) // if there are no threads, then execute everything on the main thread
	{
		for (const auto* test : tests)
			RunTest(test);
		return;
	}

	

	std::vector<std::span<const TestDefinition* const>> work_units;

	// Split the tests into different cohorts
	for (const auto* test : tests)
	{
		auto concurrency = test->_concurrency;
		concurrency = std::min(concurrency, _maximumConcurrency.value_or(concurrency));
		concurrency = _enforcedConcurrency.value_or(concurrency);
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
		float requiredBuckets = (float)remainder.size() / (MinimumNumberOfTestsPerThread);

		int remainingThreads = MaxNumberOfSimultaneousThreads - static_cast<int>(work_units.size());

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
		thread.get_stop_source().request_stop();

	// wait 100 ms
	std::this_thread::sleep_for(100ms);

	// annihlate the remaining tests.

	 
	// if the tests have not ceased, then wait 1 second
	// if the tests have not ceased, force close them. leak the memory, fuck it.
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
	TestContext context{ definition, &result };
	TestRunner::Run(context);

	// TODO: publish the result;
}

void TestCoordinator::PublishResult(const TestContext& context)
{

}