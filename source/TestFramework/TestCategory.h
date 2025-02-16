#pragma once

#include "TestDefinition.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

struct TestObject
{
	std::string Name;
	std::vector<TestDefinition> Tests;
	std::vector<std::unique_ptr<TestObject>> SubCategories;

	TestObject(const std::string& name) {
		Name = name;
	}

	TestObject(const std::string& name, std::vector<TestDefinition>&& tests)
	{
		Name = name;
		Tests = std::move(tests);
	}

	bool IsEmpty() const { return (Tests.size() + SubCategories.size()) == 0; }

	const TestObject* Add(std::unique_ptr<TestObject>&& category)
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
