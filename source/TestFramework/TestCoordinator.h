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


struct ExecutionOptions
{
	ExecutionOptions& ForceOntoMainThread() { MaxNumberOfSimultaneousThreads = 0; return *this; }

	int MaxNumberOfSimultaneousThreads = 4;
	int MinimumNumberOfTestsPerThread = 100;
	float Variance = 0.5f;
	std::chrono::milliseconds DefaultTimeOut{ 5000 };


	// allows us to enforce the concurrency type if there are problems
	std::optional<TestConcurrency> MaximumConcurrency;
	std::optional<TestConcurrency> EnforcedConcurrency;
	std::optional<std::chrono::milliseconds> MaximumTimeout;
};

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
	TestCoordinator(std::vector<const TestDefinition*> tests, const ExecutionOptions& options = ExecutionOptions());

	void Cancel();

	void RunAsyncWhenReady(std::span<const TestDefinition* const> tests, std::latch& latch);
	void RunAsyncThenDecrement(std::span<const TestDefinition* const> tests, std::latch& latch);
	void RunAsync(std::span<const TestDefinition* const> tests);

	static void RunTests(std::span<const TestDefinition* const> tests, std::stop_token token);
	static void RunTest(const TestDefinition* definition);

	static void PublishResult(const TestContext& context);

	// todo: make an unordered set?
	// can you take a span of an unordered set? probably not.
	std::vector<const TestDefinition*> _tests;
	std::array<std::vector<const TestDefinition*>, static_cast<int>(TestConcurrency::Count)> _cohorts;
	std::vector<std::jthread> _threads;
};


struct TestResult;
class test_failed;
struct TestContext
{
	const ExecutionOptions* Options;
	const TestDefinition* Definition;
	TestResult* Result;

	void SetFailure(const std::string& reason);
	void SetFailure(const test_failed& failure);

	std::chrono::milliseconds DetermineTimeout() const;
};

// This will run a test for a single thread
struct TestRunner
{
	TestRunner(std::vector<const TestDefinition*> tests, const ExecutionOptions& options = ExecutionOptions());

	bool TryRunNextTest(std::atomic<size_t>& workIndex, std::vector<const TestDefinition*>& testPool);

	static void Run(std::span<const TestDefinition* const> tests, const ExecutionOptions& options, std::stop_token token);
	static void RunAsync(TestContext context);
	static void Run(TestContext context);
};