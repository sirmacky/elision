
#include <vector>
#include <array>
#include <memory>

#include "Test_TestFramework.h"

#include "../foundation/Events.h"

// A Harness can be used to set things up in a test
// for all tests
// per test run
// pet test instance rum

// and can clean up each in between
#include "TestFramework/TestRunner.h"

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
