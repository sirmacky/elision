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
#include "TestResult.h" // needed for test_failure
#include "TestManager.h"


#define GenerateTestDeclarationName(test_name) test_name ## _test_definition

namespace lsn::test_framework::details
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

#define AssertThat(condition) lsn::test_framework::details::AssertCondition(condition, #condition, __FUNCTION__, __FILE__, __LINE__)

#define ImplementTestArguments_ValueSource(...) 
#define ImplementTestArguments_ValueCase(...) 
#define ImplementTestArguments_Arguments(...) __VA_ARGS__
#define ImplementTestArguments_WithConcurrency(...)
#define ImplementTestArguments_Timeout(...)

#define ImplementTestDataSource_ValueSource(...) .AddTestsFromSource( []() { return __VA_ARGS__ ();} )
#define ImplementTestDataSource_ValueCase(...) .AddTestsFromValues(__VA_ARGS__)
#define ImplementTestDataSource_Arguments(...)
#define ImplementTestDataSource_WithConcurrency(...)
#define ImplementTestDataSource_Timeout(...)

#define ImplementTestRequirements_ValueSource(...)
#define ImplementTestRequirements_ValueCase(...)
#define ImplementTestRequirements_Arguments(...)
#define ImplementTestRequirements_WithConcurrency(...) .SetRequirement(__VA_ARGS__)
#define ImplementTestRequirements_Timeout(...) .SetTimeout(__VA_ARGS__)


#define DeclareTest_Internal(category, test_name, ...) void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)); \
static volatile const auto* GenerateTestDeclarationName(test_name) = category->Add(lsn::test_framework::TestGenerator<decltype(test_name)>(&test_name, #test_name, __FILE__, __LINE__) \
FOR_EACH_MACRO(ImplementTestDataSource_, __VA_ARGS__) \
FOR_EACH_MACRO(ImplementTestRequirements_, __VA_ARGS__) \
.Generate()); \
inline static void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)) 

#define DeclareTestSubCategory(parent, name) namespace name {lsn::test_framework::TestObject* Category = parent::Category->Add(#name); } namespace name
#define DeclareTestCategory(name) namespace name { TestObject* Category = lsn::test_framework::TestManager::Instance().Add(#name); } namespace name
#define DeclareTest(...) DeclareTest_Internal( Category, __VA_ARGS__)

namespace lsn::test_framework
{
	namespace tuple_utils
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

	template <typename signature>
	struct TestGenerator {};

	template<typename signature>
	struct TestGeneratorBase
	{
	protected:
		// Test Object Details
		std::string _name;
		std::string _file;
		int _lineNumber;

		// Test Definition Details
		TestConcurrency _concurrency = TestConcurrency::Any;
		std::chrono::milliseconds _timeout{ 0 };

	public:

		TestGeneratorBase(const std::string& name, const std::string& file, int lineNumber)
		{
			_name = name;
			_file = file;
			_lineNumber = lineNumber;
		}

		// options
		TestGenerator<signature>& SetRequirement(TestConcurrency val)
		{
			_concurrency = val;
			return *(static_cast<TestGenerator<signature>*>(this));
		}

		TestGenerator<signature>& SetTimeout(std::chrono::milliseconds timeout)
		{
			_timeout = timeout;
			return *(static_cast<TestGenerator<signature>*>(this));
		}

		std::unique_ptr<TestDefinition> GenerateTestDefinition(std::function<void()> test_func) const
		{
			auto definition = std::make_unique<TestDefinition>(test_func);
			SetDetails(definition.get());
			return definition;
		}

		std::unique_ptr<TestObject> GenerateTestObject(std::function<void()> test_func) const
		{
			return GenerateTestObject(this->_name, test_func);
		}

		std::unique_ptr<TestObject> GenerateTestObject(std::string name, std::function<void()> test_func) const
		{
			auto test = std::make_unique<TestObject>(name, GenerateTestDefinition(test_func));
			SetDetails(test.get());
			return test;
		}

		void SetDetails(TestObject* test) const
		{
			test->File = _file;
			test->LineNumber = _lineNumber;
		}

		void SetDetails(TestDefinition* definition) const
		{
			definition->Concurrency = _concurrency;
			definition->Timeout = _timeout;
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

		std::unique_ptr<TestObject> Generate() { return this->GenerateTestObject(_test); }
	};

	template <typename R, typename ...Args>
	struct TestGenerator<R(Args...)> : public TestGeneratorBase<R(Args...)>
	{
		using ArgStorage = std::tuple<Args...>;
		using ArgVector = std::vector<ArgStorage>;

		std::function<void(Args...)> _test;
		std::vector<ArgStorage> _arguments;

		//TestGenerator((*test)(Args...), const std::string& name, const std::string& file, int lineNumber)
		TestGenerator(std::function<void(Args...)> test, const std::string& name, const std::string& file, int lineNumber)
			: TestGeneratorBase<R(Args...)>(name, file, lineNumber)
		{
			_test = test;
		}
	
		TestGenerator& AddTestsFromSource(const auto& generator)
		{
			auto arguments = std::invoke(generator);
			_arguments.reserve(arguments.size() + _arguments.size());
			for (const auto& argument : arguments)
				_arguments.push_back(argument);

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

		TestGenerator& AddTestsFromValue(const ArgStorage& args)
		{
			_arguments.push_back(args);
			return *this;
		}

		std::unique_ptr<TestObject> Generate()
		{
			assert(_arguments.size() > 0);

			auto root = std::make_unique<TestObject>(this->_name);
			this->SetDetails(root.get());

			for (const auto& arguments : _arguments)
			{
				auto instanceTest = [localTest = _test, arguments]()
				{
					return std::apply(localTest, arguments);
				};
				
				root->Add(this->GenerateTestObject(GenerateTestName(arguments), instanceTest));
			}

			return root;
		}

		std::string GenerateTestName(const ArgStorage& arguments) const {
			return std::format("{0}({1})", this->_name, GenerateArgName(arguments));
		}

		std::string GenerateArgName(const ArgStorage& arguments) const {
			return tuple_utils::to_string(arguments);
		}
	};
}