#include "TestRunner.h"



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
	TestRunner::Run(TestContext{ &test, EditResult(&test) });
}

void TestManager::Run(const TestCategory& category)
{
	for (const auto& test : category.Tests)
		Run(test);
}

/*
void TestManager::Run(const std::vector<const TestDefinition&>& tests)
{
	// TODO: Threading with test runners
	// TODO: Protect definitions if they're already running a test.
	for (const auto& test : tests)
		std::invoke(test._test);
}
*/

