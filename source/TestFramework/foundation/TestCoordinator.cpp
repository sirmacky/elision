#include "TestCoordinator.h"

#include "TestRunner.h"

#include <thread>

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

		// TODO: this is going to sheer off the last one, so add one
		// NOTE: this means that testsPerThread * numThreads != remainder.size()
		int testsPerThread = (remainder.size() / numThreads) + 1;

		// split them into sub ranges of our cohort and create a thread for each one
		for (int i = 0; i < numThreads; ++i)
		{
			auto start = (remainder.begin() + i * testsPerThread);
			RunAsync(std::span(start, testsPerThread));
		}
	}

	// TODO: once the other threads have completed, then we can run exclusive
	auto& exclusives = _cohorts[static_cast<int>(TestConcurrency::Exclusive)];
	if (exclusives.size() > 0)
	{
		RunAsync(std::span(exclusives));
	}
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
	});
}

void TestCoordinator::RunTest(const TestDefinition* definition)
{
	TestResult result;
	TestContext context{ definition, &result };
	TestRunner::Run(context);

	// TODO: publish the result;
}