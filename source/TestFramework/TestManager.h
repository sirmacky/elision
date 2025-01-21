#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "TestResult.h"
#include "TestDefinition.h"
#include "TestRunner.h"

// TODO:
// Have the definitions stored in a TestDataStore rather than the manager
// TestDefinitions should not be stored on the category itself.
// Categories should have some way of initializing components

// TODO: Remove and use an expose macro for the imgui
#include <XEnum.h>
ImplementXEnum(TestResultStatus,
	XValue(Passed),
	XValue(NotRun),
	XValue(Failed),
	XValue(WaitingToRun),
	XValue(Running)
)

struct TestManager
{
	static TestManager& Instance()
	{
		static TestManager _instance;
		return _instance;
	}

	TestExecutionOptions TestOptions;
	std::vector<TestCategory> _categories;

	TestCategory* Add(const std::string& name)
	{
		return &(_categories.emplace_back(name));

		// TODO: :Add to the results map?
	}

	void RunAll();
	void Run(const TestDefinition& definition);
	void Run(const TestCategory& category);
	void Run(const std::unordered_set<const TestDefinition*> tests);

	TestResultStatus DetermineStatus(const TestCategory& category) const;
	TestResultStatus DetermineStatus(const TestDefinition* definition) const;
	TestResultStatus DetermineStatus(const TestResult* result) const;
	bool IsQueued(const TestDefinition* definition) const;

	std::unordered_set<const TestDefinition*> Query()
	{
		// TODO: Support a string based query to query definition to get the test definitions
		return std::unordered_set<const TestDefinition*>();
	}


	const TestResult* FetchResult(const TestDefinition* definition) const
	{
		return const_cast<TestManager*>(this)->EditResult(definition);
	}

private:

	TestResult* EditResult(const TestDefinition* definition)
	{
		if (auto iter = _testResults.find(definition->_id); iter != _testResults.end())
			return &iter->second;
		return &(_testResults[definition->_id] = TestResult());
	}

	TestRunner _testRunner;

	// TODO: The key should be the definition, not the string, when we save/load from disk it can be checked via query
	std::unordered_map<std::string, TestResult> _testResults;
};
