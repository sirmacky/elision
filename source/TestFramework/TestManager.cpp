#include "TestManager.h"
#include "TestRunner.h"
#include "foundation/utils/StringUtils.h"

TestResultStatus TestManager::DetermineStatus(const TestObject* category) const
{
	TestResultStatus categoryStatus = TestResultStatus::Passed;

	if (category->Definition)
	{
		categoryStatus = DetermineStatus(category->Definition.get());
	}

	for (const auto& test : category->Children)
	{
		categoryStatus = std::max(categoryStatus, DetermineStatus(test.get()));
	}

	return categoryStatus;
}

TestResultStatus TestManager::DetermineStatus(const TestDefinition* definition) const
{
	const auto* result = FetchResult(definition);

	if (IsQueued(definition))
	{
		if (!result->HasStarted())
		{
			return TestResultStatus::WaitingToRun;
		}
		else if (!result->HasEnded())
		{
			return TestResultStatus::Running;
		}
	}

	if (!result->HasRun())
	{
		return TestResultStatus::NotRun;
	}

	return result->HasPassed() ? TestResultStatus::Passed : TestResultStatus::Failed;
	
}

bool TestManager::IsQueued(const TestDefinition* test) const
{
	return _testRunner.IsScheduled(test);
}

std::unordered_set<const TestObject*> TestManager::Query(const TestQuery& query) const
{
	std::unordered_set<const TestObject*> results; // = Query();
	
	
	//

	// this should be last as it's expensive
	if (!query.StrMatch.empty())
	{
		/*
		results.erase(std::remove_if(results.begin(), results.end(), [&](const TestObject* obj)
		{
			return StringUtils::Search(obj->Name, query.StrMatch) == 0;
		}));
		*/
	}

	return results;
}

void TestManager::RunAll()
{
	std::unordered_set<const TestDefinition*> tests;
	for (const auto& category : _categories)
	{
		category.VisitAllTests([&](const TestDefinition* test)
		{
			tests.insert(test);
		});
	}

	Run(tests);
}

void TestManager::Run(const TestObject& category)
{
	std::unordered_set<const TestDefinition*> tests;
	category.VisitAllTests([&](const TestDefinition* test) 
	{
		tests.insert(test); 
	});

	Run(tests);
}

void TestManager::Run(const TestDefinition& test)
{
	Run({ &test });
}

void TestManager::Run(const std::unordered_set<const TestDefinition*> tests)
{
	std::vector<TestContext> contexts;
	contexts.reserve(tests.size());
	for (const auto* test : tests)
		contexts.emplace_back( test, EditResult(test));

	_testRunner.Run(contexts, TestOptions);
}