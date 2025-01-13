#include "TestManager.h"
#include "TestRunner.h"
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