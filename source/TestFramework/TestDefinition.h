#pragma once

#include <string>
#include <functional>
#include <chrono>

enum class TestConcurrency
{
	Exclusive, // This test must be run exclusively against all others
	Privelaged, // only run one of these at a time. 
	Any, // this test can be run on any thread any time
	Count,
};

struct TestCategory;

struct TestDefinition
{
	TestDefinition(const std::string& name, int lineNumber, const std::function<void()>& test)
	{
		_id = name;
		_name = name;
		_lineNumber = lineNumber;
		_test = test;
	}

	TestDefinition(TestDefinition&&) = default;
	TestDefinition(const TestDefinition&) = default;
	TestDefinition() = default;

	// the data
	std::string _id;
	std::string _name;
	std::string _file;
	std::function<void()> _test;
	int _lineNumber;
	const TestCategory* _category = nullptr;
	TestConcurrency _concurrency = TestConcurrency::Any;
	std::chrono::milliseconds _timeout{ 0 }; // default timeout
};