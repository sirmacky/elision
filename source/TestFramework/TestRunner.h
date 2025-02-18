#pragma once

#include <optional>
#include <vector>
#include <array>
#include <span>
#include <unordered_set>
#include <thread>

#include "TestDefinition.h"

namespace lsn::test_framework
{
	struct TestResult;
	class test_failure;

	struct TestExecutionOptions
	{
		TestExecutionOptions& ForceOntoMainThread() { MaxNumberOfSimultaneousThreads = 0; return *this; }

		int MaxNumberOfSimultaneousThreads = 6;
		int MinimumNumberOfTestsPerThread = 2;
		std::chrono::milliseconds DefaultTimeOut{ 5000 };


		// allows us to enforce the concurrency type if there are problems
		std::optional<TestConcurrency> MaximumConcurrency;
		std::optional<TestConcurrency> EnforcedConcurrency;
		std::optional<std::chrono::milliseconds> MaximumTimeout;
	};


	struct TestContext
	{
		// TODO: Should this be the object?
		const TestDefinition* Definition;
		TestResult* Result;

		void SetFailure(const std::string& reason);
		void SetFailure(const test_failure& failure);

		std::chrono::milliseconds DetermineTimeout(const TestExecutionOptions& options) const;
	};

	struct TestRunner
	{
		TestRunner() = default;
		~TestRunner();

		enum class Status
		{
			Idle,
			Running,
			Cancelling,
		};

		std::atomic<Status> Status{ Status::Idle };

		bool IsScheduled(const TestDefinition* test) const;

		void Run(std::vector<TestContext>& tests, const TestExecutionOptions& options = TestExecutionOptions());
		void Cancel();
		void Join();

		std::unordered_set<const TestDefinition*> _tests;
		std::stop_source _stopSource{};
		std::thread _thread;

		static void RunAll(std::vector<TestContext>& tests, const TestExecutionOptions& options, std::stop_token token);
		static void RunAsync(std::span<TestContext* const> tests, const TestExecutionOptions& options, std::stop_token token);
		static void Run(TestContext context, const TestExecutionOptions& options, std::stop_token token);
	private:
		static void RunInternal(TestContext& context, const TestExecutionOptions& options);

		void OnFinish();
	};
}