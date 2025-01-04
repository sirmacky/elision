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

#include <thread>
#include <latch>

#include "TestDefinition.h"
#include "unordered_set"

class TestContext;
struct TestCoordinator
{
	enum class Status
	{
		Idle,
		Running,
		Cancelling,
		Done,
	};

	Status Status = Status::Idle;

	// TODO: make configurable
	void ForceOntoMainThread() { MaxNumberOfSimultaneousThreads = 0; }
	int MaxNumberOfSimultaneousThreads = 4;
	int MinimumNumberOfTestsPerThread = 100;
	float Variance = 0.5f;

	// allows us to enforce the concurrency type if there are problems
	std::optional<TestConcurrency> _maximumConcurrency;
	std::optional<TestConcurrency> _enforcedConcurrency;

	TestCoordinator()

	void Run(std::vector<const TestDefinition*> tests);
	void Cancel();

	void RunAsyncWhenReady(std::span<const TestDefinition* const> tests, std::latch& latch);
	void RunAsyncThenDecrement(std::span<const TestDefinition* const> tests, std::latch& latch);

	static void RunTests(std::span<const TestDefinition* const> tests, std::stop_token token);
	static void RunTest(const TestDefinition* definition);

	static void PublishResult(const TestContext& context);

	std::array<std::vector<const TestDefinition*>, static_cast<int>(TestConcurrency::Count)> _cohorts;
	std::vector<std::jthread> _threads;
	std::latch ExclusiveLatch;
};