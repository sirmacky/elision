#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "TestResult.h"
#include "TestDefinition.h"

// TODO:
// Have the definitions stored in a TestDataStore rather than the manager
// TestDefinitions should not be stored on the category itself.
// Categories should have some way of initializing components

struct TestManager
{
	static TestManager& Instance()
	{
		static TestManager _instance;
		return _instance;
	}

	std::vector<TestCategory> _categories;

	TestCategory* Add(const std::string& name)
	{
		return &(_categories.emplace_back(name));
	}

	void RunAll();
	void Run(const TestDefinition& definition);
	void Run(const TestCategory& category);
	//void Run(const std::vector<const TestDefinition&>& tests);

	std::unordered_set<const TestDefinition*> Query()
	{
		// TODO: Support a string based query to query definition to get the test definitions
		return std::unordered_set<const TestDefinition*>();
	}

	

	const TestResult* FetchResult(const TestDefinition* definition) const
	{
		return const_cast<TestManager*>(this)->EditResult(definition);

	}
	TestResult* EditResult(const TestDefinition* definition)
	{
		if (auto iter = _testResults.find(definition->_id); iter != _testResults.end())
			return &iter->second;
		return &(_testResults[definition->_id] = TestResult());
	}

private:

	TestResultStatus DetermineStatus(const TestCategory& category) const;
	std::unordered_map<std::string, TestResult> _testResults;
};
