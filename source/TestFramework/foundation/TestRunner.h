#pragma once

#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <XEnum.h>
#include <iostream>
#include <format>
#include <unordered_map>
#include <chrono>
#include <tuple>
#include <type_traits>
#include <cassert>



// TODO:
// Have the definitions stored in a TestDataStore rather than the manager
// TestDefinitions should not be stored on the category itself.
// Categories should have some way of initializing components





enum class TestConcurrency
{
	Exclusive, // This test must be run exclusively
	Privelaged, // only run one of these at a time. 
	Any, // this test can be run on any thread any time
};

struct TestCategory;

struct TestDefinition
{
	TestDefinition(const std::string& name, int lineNumber, const std::function<void()>& test)
	{
		_id = name;
		_name = name;
		_lineNumber = lineNumber;
		_test = test;
	}

	TestDefinition(TestDefinition&&) = default;
	TestDefinition(const TestDefinition&) = default;
	TestDefinition() = default;

	// the data
	std::string _id;
	std::string _name;
	std::string _file;
	std::function<void()> _test;
	int _lineNumber;
	const TestCategory* _category = nullptr;
	TestConcurrency _concurrency = TestConcurrency::Any;
};

class test_failed
{
public:
	test_failed() = default;
	test_failed(const test_failed&) = default;

	test_failed(const std::string& error, const std::string& filename, int errorLine)
	{
		_error = error;
		_filename = filename;
		_errorLine = errorLine;
	}

	std::string FormattedString() const {
		return std::format("{0} in {1}:{2}", _error, _filename, _errorLine);
	}

	friend std::ostream& operator<<(std::ostream& os, const test_failed& dt) {
		return (os << dt.FormattedString());
	}

	const std::string& error() const { return _error; }
	const std::string& filename() const { return _filename; }
	int linenumber() const { return _errorLine; }

private:

	std::string _error;
	std::string _filename;
	int _errorLine;
};

ImplementXEnum(TestResultStatus,
	XValue(Passed),
	XValue(NotRun),
	XValue(Failed))

struct TestResult
{
	std::chrono::nanoseconds _timeStarted;
	std::chrono::nanoseconds _timeEnded;
	std::optional<test_failed> _lastFailure;

	void Reset()
	{
		_lastFailure.reset();
		_timeEnded = {};
		_timeStarted = {};
	}

	void Begin(std::chrono::nanoseconds timeStarted) {
		_timeStarted = timeStarted;
	}

	void SetFailure(const test_failed& failure)
	{
		_lastFailure = failure;
	}

	void End(std::chrono::nanoseconds timeEnded) {
		_timeEnded = timeEnded;
	}

	std::chrono::nanoseconds TimeTaken() const {
		return _timeEnded - _timeStarted;
	}

	bool HasRun() const {
		return _timeEnded.count() > 0;
	}

	bool HasPassed() const {
		return !_lastFailure.has_value();
	}

	TestResultStatus Status() const
	{
		if (!HasRun())
			return TestResultStatus::NotRun;

		return HasPassed() ? TestResultStatus::Passed : TestResultStatus::Failed;
	}

	operator bool() const { return HasPassed(); }
};

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
};


struct TestManager
{
	static TestManager& Instance()
	{
		static TestManager _instance;
		return _instance;
	}

	std::vector<TestCategory> _categories;

	TestCategory* Add(const std::string& name)
	{
		return &(_categories.emplace_back(name));
	}



	void Run(const TestDefinition& definition);
	void Run(const TestCategory& category);
	//void Run(const std::vector<const TestDefinition&>& tests);

	// TODO: Set?
	std::vector<const TestDefinition*> Query()
	{
		// TODO: Support a string based query to query definition to get the test definitions
		return std::vector<const TestDefinition*>();
	}

	const TestResult* FetchResult(const TestDefinition* definition) const
	{
		return const_cast<TestManager*>(this)->EditResult(definition);

	}
	TestResult* EditResult(const TestDefinition* definition)
	{
		if (auto iter = _testResults.find(definition->_id); iter != _testResults.end())
			return &iter->second;
		return &(_testResults[definition->_id] = TestResult());
	}

private:

	TestResultStatus DetermineStatus(const TestCategory& category) const;
	std::unordered_map<std::string, TestResult> _testResults;
};

struct TestContext
{
	const TestDefinition* Definition;
	TestResult* Result;
};

// This will run a test for a single thread
struct TestRunner
{
	static void Run(TestContext context)
	{
		context.Result->Reset();

		try
		{
			context.Result->Begin(std::chrono::high_resolution_clock::now().time_since_epoch());
			std::invoke(context.Definition->_test);
		}
		catch (test_failed failure)
		{
			context.Result->SetFailure(failure);
			// TODO:
		}
		catch (std::exception unexpected_failure)
		{
			context.Result->SetFailure(test_failed(unexpected_failure.what(), context.Definition->_file, context.Definition->_lineNumber));
		}
		catch (...) // unknown failure
		{
			context.Result->SetFailure(test_failed("uknown exception encountered", context.Definition->_file, context.Definition->_lineNumber));
		}

		context.Result->End(std::chrono::high_resolution_clock::now().time_since_epoch());
	}
};


#define DeclareTestCategory(categoryName) TestCategory* categoryName = TestManager::Instance().Add(#categoryName);
#define GenerateTestDeclarationName(test_name) test_name ## _test_definition

namespace Tests::details
{
	static inline bool AssertCondition(bool condition, const char* condition_str, const char* function, const char* file, int lineNumber)
	{
		if (!condition)
		{
			// TODO: We need to stop the timer for the current executing test on our thread
			// as throwing the exception will add significant time
			throw test_failed(condition_str, file, lineNumber);
		}
			
		return true;
	}
}

#define AssertThat(condition) Tests::details::AssertCondition(condition, #condition, __FUNCTION__, __FILE__, __LINE__)

#define ImplementTestArguments_ValueSource(...) 
#define ImplementTestArguments_ValueCase(...) 
#define ImplementTestArguments_Arguments(...) __VA_ARGS__
#define ImplementTestArguments_WithRequirement(...)

#define ImplementTestDataSource_ValueSource(source) .AddTestsFromSource( []() { return source ();} )
#define ImplementTestDataSource_ValueCase(...) .AddTestsFromValues(__VA_ARGS__)
#define ImplementTestDataSource_Arguments(...)
#define ImplementTestDataSource_WithRequirement(...)

#define ImplementTestRequirements_ValueSource(source)
#define ImplementTestRequirements_ValueCase(...)
#define ImplementTestRequirements_Arguments(...)
#define ImplementTestRequirements_WithRequirement(...) .SetRequirement(__VA_ARGS__)


#define DeclareTest(category, test_name, ...) void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)); \
static volatile const auto* GenerateTestDeclarationName(test_name) = category->Add(TestGenerator<decltype(test_name)>(&test_name, #test_name, __FILE__, __LINE__) \
FOR_EACH_MACRO(ImplementTestDataSource_, __VA_ARGS__) \
FOR_EACH_MACRO(ImplementTestRequirements_, __VA_ARGS__) \
.Generate()); \
void test_name (FOR_EACH_MACRO(ImplementTestArguments_, __VA_ARGS__)) 

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

	TestDefinition GenerateTest(std::function<arg> test_func)
	{
		auto definition = TestDefinition(_name, _lineNumber, test_func);
		SetRequirements(definition);
		return definition;
	}

	void SetRequirements(TestDefinition& definition)
	{
		definition._concurrency = _concurrency;
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