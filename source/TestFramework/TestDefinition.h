#pragma once


#include <functional>
#include <chrono>

enum class TestConcurrency
{
	Exclusive, // This test must be run exclusively against all others
	Privelaged, // only run one of these at a time. 
	Any, // this test can be run on any thread any time
	Count,
};

struct TestObject;

struct TestDefinition
{
	TestDefinition(const std::function<void()>& test)
	{
		_test = test;
	}

	TestDefinition(TestDefinition&&) = default;
	TestDefinition(const TestDefinition&) = default;
	TestDefinition() = default;

	std::function<void()> _test;
	const TestObject* _parent = nullptr;
	TestConcurrency Concurrency = TestConcurrency::Any;
	std::chrono::milliseconds Timeout{ 0 }; // default timeout
};

#include <string>
#include <vector>
#include <memory>
#include <functional>

struct TestObject
{
	TestObject* Parent{ nullptr };
	
	std::function<void()> Initialize;

	// TODO: Should these be a discriminated union?
	std::vector<std::unique_ptr<TestObject> > Children;
	std::unique_ptr<TestDefinition> Definition;

	std::function<void()> TearDown;

	std::string Id;
	std::string Name;
	std::string File;
	int LineNumber{ 0 };

	
	TestObject(const std::string& name) 
	{
		Id = name;
		Name = name; 
	}

	TestObject(const std::string& name, std::unique_ptr<TestDefinition> test)
		: TestObject(name)
	{
		Definition = std::move(test);
		Definition->_parent = this;
	}

	TestObject(const std::string& name, std::vector<std::unique_ptr<TestObject>>&& children)
		: TestObject(name)
	{
		Children = std::move(children);
		for (auto& child : Children)
			child->Parent = this;
	}

	TestObject(TestObject&&) = default;
	TestObject(const TestObject&) = default;
	TestObject() = default;

	TestObject* Add(std::unique_ptr<TestObject>&& child)
	{
		auto* test = Children.emplace_back(std::move(child)).get();
		test->Parent = this;
		return test;
	}

	// TODO: Move to cpp
	void VisitAllTests(std::function<void(const TestObject*)> visitor) const
	{
		for (const auto& test : Children)
		{
			test->VisitAllTests(visitor);
			std::invoke(visitor, test.get());
		}
	}

	// TODO: Move to cpp
	void VisitAllTests(std::function<void(const TestDefinition*)> visitor) const
	{
		for (const auto& test : Children)
		{
			test->VisitAllTests(visitor);
		}

		if (Definition)
			std::invoke(visitor, Definition.get());
	}

	const TestObject* GetRoot() const
	{
		const TestObject* root = this;
		while (root->Parent)
			root = root->Parent;
		return root;
	}
};
