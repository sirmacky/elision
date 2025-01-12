#pragma once

#include "TestDefinition.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct TestCategory
{
	// TODO: Move this to a centralized container

	std::string Name;
	std::vector<TestDefinition> Tests;
	std::vector<std::unique_ptr<TestCategory>> SubCategories;

	TestCategory(const std::string& name) {
		Name = name;
	}

	TestCategory(const std::string& name, std::vector<TestDefinition>&& tests)
	{
		Name = name;
		Tests = std::move(tests);
	}

	bool IsEmpty() const { return (Tests.size() + SubCategories.size()) == 0; }

	const TestCategory* Add(std::unique_ptr<TestCategory>&& category)
	{
		return SubCategories.emplace_back(std::move(category)).get();
	}

	const TestDefinition* Add(const TestDefinition& instance)
	{
		Tests.push_back(instance);
		return &Tests.back();
	}

	template<typename...Args>
	const TestDefinition* Add(Args... args)
	{
		return &(Tests.emplace_back(args...));
	}

	void VisitAllTests(std::function<void(const TestDefinition*)> visitor) const
	{ 
		for (const auto& test : Tests)
			std::invoke(visitor, &test);

		for (const auto& category : SubCategories)
			category->VisitAllTests(visitor);
	}

};
