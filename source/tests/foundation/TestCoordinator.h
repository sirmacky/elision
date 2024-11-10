#pragma once

// given a cohort of tests, this will ensure they are all run

// So what behaviour do we want from the test co-ordinator

// It should schedule tests concurrently to the best of it's ability
// it should support std::stop_token
// it should track what tests are currently running (including those that havent responded to the stop token)
// it should synchronise the results that it can semi-regularly
// it should monitor and support time outs

#include <optional>
#include <vector>
#include <array>
#include <span>

class TestDefinition;

struct TestCoordinator
{
	const int MaxNumberOfThreads = 4;
	const int MinimumNumberOfTestsPerThread = 100;
	const float Variance = 0.5f;

	// allows us to enforce the concurrency type if there are problems
	std::optional<TestConcurrency> _maximumConcurrency;
	std::optional<TestConcurrency> _enforcedConcurrency;
	bool _forceMainThread = false;

	void Run(std::vector<const TestDefinition*> tests);
	void Cancel() {} // TODO:

	void RunAsync(std::span<const TestDefinition* const> tests);

	static void RunTest(const TestDefinition* definition);
};