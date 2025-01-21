#include "TestManager.h"
#include "TestRunner.h"
#include "TestCategory.h"

TestResultStatus TestManager::DetermineStatus(const TestCategory& category) const
{
	TestResultStatus categoryStatus = TestResultStatus::Passed;

	for (const auto& test : category.Tests)
	{
		categoryStatus = std::max(categoryStatus, DetermineStatus(&test));
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

void TestManager::RunAll()
{
	std::unordered_set<const TestDefinition*> tests;
	for (const auto& category : _categories)
		category.VisitAllTests([&](const TestDefinition* test) {tests.insert(test); });

	Run(tests);
}

void TestManager::Run(const TestDefinition& test)
{
	Run({ &test });
}

void TestManager::Run(const TestCategory& category)
{
	std::unordered_set<const TestDefinition*> tests;
	category.VisitAllTests([&](const TestDefinition* test) {tests.insert(test); });
	Run(tests);
}

void TestManager::Run(const std::unordered_set<const TestDefinition*> tests)
{
	std::vector<TestContext> contexts;
	contexts.reserve(tests.size());
	for (const auto* test : tests)
		contexts.emplace_back( test, EditResult(test));

	_testRunner.Run(contexts, TestOptions);
}