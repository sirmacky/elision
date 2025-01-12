#include "TestManager.h"
#include "TestCoordinator.h"
#include "TestCategory.h"

TestResultStatus TestManager::DetermineStatus(const TestCategory& category) const
{
	TestResultStatus categoryStatus = TestResultStatus::Passed;

	for (const auto& test : category.Tests)
	{
		const auto* result = FetchResult(&test);
		categoryStatus = std::max(categoryStatus, result->Status());
	}

	return categoryStatus;
}

void TestManager::Run(const TestDefinition& test)
{
	TestRunner::Run({ &test });
}

void TestManager::Run(const TestCategory& category)
{
	std::unordered_set<const TestDefinition*> tests;
	category.VisitAllTests([&](const TestDefinition* test) {tests.insert(test); });
	
	TestRunner _runner;
	_runner.Run(tests);
	_runner.Join();
}