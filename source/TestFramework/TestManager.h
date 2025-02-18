#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <bitset>

#include "TestResult.h"
#include "TestObject.h"
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

// this system doesnt need to be embedded, and can be done at a higher layer.
namespace lsn::test_framework
{
	struct TestQuery
	{
		std::string StrMatch;
		std::bitset<XEnumTraits<TestResultStatus>::Count> StatusMask{ ~0ULL };
	};

	struct TestManager
	{
		static TestManager& Instance()
		{
			static TestManager _instance;
			return _instance;
		}

		TestExecutionOptions TestOptions;
		std::vector<TestObject> _categories;

		TestObject* Add(const std::string& name)
		{
			return &(_categories.emplace_back(name));
		}

		void RunAll();
		void Run(const TestObject& category);
		void Run(const TestDefinition& definition);
		void Run(const std::unordered_set<const TestDefinition*> tests);

		bool IsRunningTests() const;
		bool Cancel();

		TestResultStatus DetermineStatus(const TestObject* category) const;
		TestResultStatus DetermineStatus(const TestDefinition* definition) const;
	
		bool IsQueued(const TestDefinition* definition) const;

		// TODO: Potentially change to a database for the results?
		std::unordered_set<const TestDefinition*> Query()
		{
			// TODO: Support a string based query to query definition to get the test definitions
			return std::unordered_set<const TestDefinition*>();
		}

		std::unordered_set<const TestObject*> Query(const TestQuery& query) const;

		const TestResult* FetchResult(const TestDefinition* definition) const
		{
			return const_cast<TestManager*>(this)->EditResult(definition);
		}

		const TestResult* FetchResult(const TestObject* object) const
		{
			return const_cast<TestManager*>(this)->EditResult(object);
		}

	private:

		TestResult* EditResult(const TestObject* object)
		{
			if (auto iter = _testResults.find(object->Id); iter != _testResults.end())
				return &iter->second;
			return &(_testResults[object->Id] = TestResult());
		}

		TestResult* EditResult(const TestDefinition* definition)
		{
			return EditResult(definition->_parent);
		}

		TestRunner _testRunner;

		// TODO: The key should be the definition, not the string, when we save/load from disk it can be checked via query
		std::unordered_map<std::string, TestResult> _testResults;
	};
}