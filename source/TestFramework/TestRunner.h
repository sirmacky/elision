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
#include <unordered_set>

#include <thread>

#include "TestDefinition.h"


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


struct TestResult;
class test_failure;

struct TestContext
{
	const TestDefinition* Definition;
	TestResult* Result;

	void SetFailure(const std::string& reason);
	void SetFailure(const test_failure& failure);

	std::chrono::milliseconds DetermineTimeout(const ExecutionOptions& options) const;
};

// TODO: the test runner should work on contexts
// TODO: the TestContext should not have the options on it.

// This will run a test for a single thread
struct TestRunner
{
	enum class Status
	{
		Idle,
		Running,
		Cancelling,
		Done,
	};

	Status Status = Status::Idle;

	void Run(std::unordered_set<const TestDefinition*> tests, const ExecutionOptions& options = ExecutionOptions());
	void Cancel();
	void Join();

	std::stop_source _stopSource{};
	std::thread _thread;

	static void Run(std::unordered_set<const TestDefinition*> tests, const ExecutionOptions& options, std::stop_token token);
	static void RunAsync(std::span<const TestDefinition* const> tests, const ExecutionOptions& options, std::stop_token token);
	static void Run(const TestDefinition* const definition, const ExecutionOptions& option = ExecutionOptions());
	static void Run(TestContext context, const ExecutionOptions& option = ExecutionOptions());
private:
	static void RunInternal(TestContext context, const ExecutionOptions& options);
};