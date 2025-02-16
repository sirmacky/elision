#pragma once

#include <functional>
#include <chrono>

namespace lsn::test_framework
{
	enum class TestConcurrency
	{
		Exclusive, // This test must be run exclusively against all others
		Privelaged, // only run one of these at a time. 
		Any, // this test can be run on any thread any time
		Count,
	};

	struct TestObject;

	struct TestDefinition
	{
		TestDefinition(const std::function<void()>& test)
		{
			_test = test;
		}

		TestDefinition(TestDefinition&&) = default;
		TestDefinition(const TestDefinition&) = default;
		TestDefinition() = default;

		std::function<void()> _test;
		const TestObject* _parent = nullptr;
		TestConcurrency Concurrency = TestConcurrency::Any;
		std::chrono::milliseconds Timeout{ 0 }; // default timeout
	};
}

