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
	if (_forceMainThread)
	{
		for (const auto* test : tests)
			RunTest(test);
		return;
	}

	std::array<std::vector<const TestDefinition*>, 3> _cohorts;

	// Split the tests into different cohorts
	for (const auto* test : tests)
	{
		auto concurrency = test->_concurrency;
		concurrency = std::min(concurrency, _maximumConcurrency.value_or(concurrency));
		concurrency = _enforcedConcurrency.value_or(concurrency);
		_cohorts[static_cast<int>(concurrency)].push_back(test);
	}

	int consumedThreads = 0;

	// create a thread for the privelaged
	// these can conflict with one another
	const auto& privelaged = _cohorts[static_cast<int>(TestConcurrency::Privelaged)];

	if (privelaged.size() > 0)
	{
		// TODO: create a thread and increment the consumed thread count
		RunAsync(std::span(privelaged));
		consumedThreads++;
	}

	// determine buckets for the remainder
	// these can be chunked into whatever needs to be there
	const auto& remainder = _cohorts[static_cast<int>(TestConcurrency::Any)];
	if (remainder.size() > 0)
	{
		float requiredBuckets = (float)remainder.size() / (MinimumNumberOfTestsPerThread);

		// Determine the number of threads needed
		int numThreads = 0;

		// TODO: Account for the variance
		numThreads = std::max(std::min(numThreads, MaxNumberOfThreads - consumedThreads), 1); // there must be at least 1 thread
		
		// split them into sub ranges of our cohort and create a thread for each one
		int testsPerThread = (remainder.size() / numThreads);
		int numRemaining = remainder.size() % numThreads;
		
		int consumed = 0;
		for (int i = 0; i < numThreads; ++i)
		{
			int numTests = testsPerThread;
			if (i < numRemaining)
				numTests += 1;

			auto start = (remainder.begin() + consumed);
			consumed += numTests;

			RunAsync(std::span(start, numTests));
		}
	}

	// TODO: once the other threads have completed, then we can run exclusive
	auto& exclusives = _cohorts[static_cast<int>(TestConcurrency::Exclusive)];
	if (exclusives.size() > 0)
	{
		RunAsync(std::span(exclusives));
	}
}

void TestCoordinator::Cancel()
{
	// send a stop token
	// wait 100 ms
	// if the tests have not ceased, then wait 1 second
	// if the tests have not ceased, force close them. leak the memory, fuck it.

}

void TestCoordinator::RunAsync(std::span<const TestDefinition* const> tests)
{
	std::jthread thread([tests](std::stop_token token)
	{
		for (const auto* test : tests)
		{
			if (token.stop_requested())
				return;

			RunTest(test);
		}

		// reduce the barrier and consider running

	});
	


	thread.detach();
}

void TestCoordinator::RunTest(const TestDefinition* definition)
{
	TestResult result;
	TestContext context{ definition, &result };
	TestRunner::Run(context);

	// TODO: publish the result;
}