#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>

#include <iostream>
#include <format>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <cassert>

#include "TestDefinition.h"
#include "TestCategory.h"
#include "TestResult.h" // needed for test_failure
#include "TestManager.h"


#define GenerateTestDeclarationName(test_name) test_name ## _test_definition

namespace Tests::details
{
	static inline bool AssertCondition(bool condition, const char* condition_str, const char* function, const char* file, int lineNumber)
	{
		if (!condition)
		{
			// TODO: We need to stop the timer for the current executing test on our thread
			// as throwing the exception will add significant time
			throw test_failure(condition_str, file, lineNumber);
		}
			
		return true;
	}
}

#define AssertThat(condition) Tests::details::AssertCondition(condition, #condition, __FUNCTION__, __FILE__, __LINE__)

#define ImplementTestArguments_ValueSource(...) 
#define ImplementTestArguments_ValueCase(...) 
#define ImplementTestArguments_Arguments(...) __VA_ARGS__
#define ImplementTestArguments_WithRequirement(...)
#define ImplementTestArguments_Timeout(...)

#define ImplementTestDataSource_ValueSource(...) .AddTestsFromSource( []() { return __VA_ARGS__ ();} )
#define ImplementTestDataSource_ValueCase(...) .AddTestsFromValues(__VA_ARGS__)
#define ImplementTestDataSource_Arguments(...)
#define ImplementTestDataSource_WithRequirement(...)
#define ImplementTestDataSource_Timeout(...)

#define ImplementTestRequirements_ValueSource(...)
#define ImplementTestRequirements_ValueCase(...)
#define ImplementTestRequirements_Arguments(...)
#define ImplementTestRequirements_WithRequirement(...) .SetRequirement(__VA_ARGS__)
#define ImplementTestRequirements_Timeout(...) .SetTimeout(__VA_ARGS__)


#define DeclareTest_Internal(category, test_name, ...) void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)); \
static volatile const auto* GenerateTestDeclarationName(test_name) = category->Add(TestGenerator<decltype(test_name)>(&test_name, #test_name, __FILE__, __LINE__) \
FOR_EACH_MACRO(ImplementTestDataSource_, __VA_ARGS__) \
FOR_EACH_MACRO(ImplementTestRequirements_, __VA_ARGS__) \
.Generate()); \
inline static void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)) 

#define DeclareTestCategory(name) namespace name { TestCategory* Category = TestManager::Instance().Add(#name); } namespace name
#define DeclareTest(...) DeclareTest_Internal( Category, __VA_ARGS__)

namespace TupleHelpers
{
	template<class TupType, size_t... I>
	std::string to_string(const TupType& _tup, std::index_sequence<I...>)
	{
		std::stringstream s;
		(..., (s << (I == 0 ? "" : ", ") << std::get<I>(_tup)));
		return s.str();
	}

	template<class... T>
	std::string to_string(const std::tuple<T...>& _tup)
	{
		return to_string(_tup, std::make_index_sequence<sizeof...(T)>());
	}
}


template <typename arg>
struct TestGenerator {};

template<typename arg>
struct TestGeneratorBase
{
	std::string _name;
	std::string _file;
	int _lineNumber;
	TestConcurrency _concurrency = TestConcurrency::Any;
	std::chrono::milliseconds _timeout{ 0 };

	TestGeneratorBase(const std::string& name, const std::string& file, int lineNumber)
	{
		_name = name;
		_file = file;
		_lineNumber = lineNumber;
	}

	TestGenerator<arg>& SetRequirement(TestConcurrency val)
	{
		_concurrency = val;
		return *(static_cast<TestGenerator<arg>*>(this));
	}

	TestGenerator<arg>& SetTimeout(std::chrono::milliseconds timeout)
	{
		_timeout = timeout;
		return *(static_cast<TestGenerator<arg>*>(this));
	}

	TestDefinition GenerateTest(std::function<arg> test_func)
	{
		auto definition = TestDefinition(_name, _lineNumber, test_func);
		SetRequirements(definition);
		return definition;
	}

	void SetRequirements(TestDefinition& definition)
	{
		definition._concurrency = _concurrency;
		definition._timeout = _timeout;
	}
};


template <typename R>
struct TestGenerator<R()> : public TestGeneratorBase<R()>
{
	std::function<R()> _test;
	TestGenerator(std::function<R()> test, const std::string& name, const std::string& file, int lineNumber)
		: TestGeneratorBase<R()>(name, file, lineNumber)
	{
		_test = test;
	}

	TestDefinition Generate() { return this->GenerateTest(_test); }
};

template <typename R, typename ...Args>
struct TestGenerator<R(Args...)> : public TestGeneratorBase<R(Args...)>
{
	using ArgStorage = std::tuple<Args...>;
	using ArgVector = std::vector<ArgStorage>;

	std::function<void(Args...)> _test;

	std::vector<TestDefinition> _tests;

	//TestGenerator((*test)(Args...), const std::string& name, const std::string& file, int lineNumber)
	TestGenerator(std::function<void(Args...)> test, const std::string& name, const std::string& file, int lineNumber)
		: TestGeneratorBase<R(Args...)>(name, file, lineNumber)
	{
		_test = test;
	}
	
	TestGenerator& AddTestsFromSource(const auto& generator)
	{
		auto arguments = std::invoke(generator);
		_tests.reserve(arguments.size() + _tests.size());
		for (const auto& argument : arguments)
		{
			AddTestsFromValue(argument);
		}

		return *this;
	}

	TestGenerator& AddTestsFromValues(Args... args)
	{
		auto tuple = std::make_tuple(args...);
		return AddTestsFromValue(tuple);
	}

	template<typename T>
	TestGenerator& AddTestsFromValue(const T& argument)
	{
		return AddTestsFromValue(std::tuple(argument));
	}

	TestGenerator& AddTestsFromValue(const ArgStorage& arguments)
	{
		auto localTest = _test;
		auto instanceTest = [localTest, arguments]() 
		{
			return std::apply(localTest, arguments); 
		};
		auto& test = _tests.emplace_back(GenerateTestName(arguments), this->_lineNumber, instanceTest);
		this->SetRequirements(test);		
		return *this;
	}

	std::unique_ptr<TestCategory> Generate()
	{
		assert(_tests.size() > 0);
		return std::make_unique<TestCategory>(this->_name, std::move(_tests));
	}

	std::string GenerateTestName(const ArgStorage& arguments) const {
		return std::format("{0}({1})", this->_name, GenerateArgName(arguments));
	}

	std::string GenerateArgName(const ArgStorage& arguments) const {
		return TupleHelpers::to_string(arguments);
	}
};