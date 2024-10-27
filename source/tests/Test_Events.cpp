
#include <vector>
#include <array>
#include <memory>

#include "Test_Events.h"

#include "../foundation/Events.h"

// A Harness can be used to set things up in a test
// for all tests
// per test run
// pet test instance rum

// and can clean up each in between
#include "TestRunner.h"

namespace Tests::Categories
{
	DeclareTestCategory(Standard);
}

namespace Tests
{
	DeclareTest(Categories::Standard, Success) 
	{
		AssertThat(1 > 0);
	}

	DeclareTest(Categories::Standard, Failure)
	{
		AssertThat(1 == 0);
	}
}

namespace Example
{
	auto GenerateSingleData()
	{
		std::vector<int> a(10);
		for (int i = 0; i < 10; ++i)
			a[i] = i;
		return a;
	}

	auto GenerateSingleTupleData()
	{
		std::vector<std::tuple<int>> a(10);
		for (int i = 0; i < 10; ++i)
			a[i] = (std::make_tuple(i+10));
		return a;
	}

	auto GenerateMultipleTupleData()
	{
		std::vector<std::tuple<int, int>> a(10);
		for (int i = 0; i < 10; ++i)
			a[i] = (std::make_tuple(i, i + 10));
		return a;
	}
}

namespace Tests
{
	DeclareTest(Categories::Standard, TestSomething, Arguments(int a),
		ValueSource(Example::GenerateSingleData),
		ValueSource(Example::GenerateSingleTupleData),
		ValueCase(42),
		ValueCase(1337))
	{
		AssertThat(a != 1337);
	}

	DeclareTest(Categories::Standard, TestMultiple, WithRequirement(TestConcurrency::Exclusive),
		ValueSource(Example::GenerateMultipleTupleData), 
		ValueCase(42, 43),
	Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	DeclareTest(Categories::Standard, TestSomethingWithNoArgs)
	{
		AssertThat(1 > 0);
	}
}


/*
namespace Tests::Events
{

	template<typename ... Args>
	struct Tester
	{
		UnorderedEvent<Args...> _event{};

		Tester()
		{
			auto memberDispath = _event.Attach(this, &Tester::OnMemberDispatch);
			auto staticDispatch = _event.Attach(&OnStaticDispatch);
			auto lambdaDispatch = _event.Attach([](Args...) {});
		}

		void OnMemberDispatch(Args...args)
		{
			// TODO:
		}

		static inline int OnStaticDispatch(Args... args)
		{
			return 0;
		}
	};

	class Example {};

	static std::function<void()> _runAll = []() {RunAll(); };

	void RunAll()
	{
		std::tuple tests{
			Tester<int>(),
			Tester<int, int>(),
			Tester<const Example&>(),
		};


		ObservableValue<int> a;
		a = a.GetValue() + a;

		if (a < 10.0f)
		{

		}
	}
}
*/
