
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

DeclareTestCategoryV2(StandardV2)
{
	// This will allow us to set up common start and teardown operations
	// as well as a single setup function
	DeclareTestV2(TestSomething,
		ValueSource(Example::GenerateSingleData),
		ValueSource(Example::GenerateSingleTupleData),
		ValueCase(42),
		ValueCase(1337),
	Arguments(int a))
	{
		AssertThat(a != 1337);
	}

	DeclareTestV2(TestMultiple, WithRequirement(TestConcurrency::Exclusive),
		ValueSource(Example::GenerateMultipleTupleData),
		ValueCase(42, 43),
		Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	DeclareTestV2(TestSomethingWithNoArgs)
	{
		AssertThat(1 > 0);
	}
}


// TODO: V3
// OPTION 1
// in order to speed up tests there may be test dependencies that a test may want. 
// such as loading a shared resource, or something
// the cateogry should be responsible for providing and tearing down this data
// teardown should be explicit 


#define DeclareTestCategoryV3(name) namespace name

#define DeclareTestCategoryTeardown() void Teardown()
#define DeclareTestCategorySetup() void Setup()


DeclareTestCategoryV3(TestsV3)
{
	DeclareTestCategorySetup() { }
	DeclareTestCategoryTeardown() { }
}

// OPTION 2
// its possible multiple items may want to load data
// so we may need to develop a dependency graph for them
// execution of the tests is also a dependency graph in a way

#define ImplementCategoryDependency
#define DeclareTestCategoryDependencies(...) int Dependencies = { }

struct Dependency
{
	std::vector<Dependency*> Dependencies;

	template<typename...Args>
	Dependency(Args&&... args)
	{
		Dependencies.push_back(args...)
	}
	Dependency() = default;

	// Resolve will be called when all dependncies are ready.
	virtual void Resolve() = 0;
};

struct FileLoaderDependency : Dependency
{
	virtual void Resolve() override
	{

	}
};


template<typename T>
struct StandardResourceDependency : Dependency
{
	T* Resource;
	// overload the -> operator to access the resource

	T& operator*() { return Resource; }
	const T& operator*() const { return Resource; }

	T* operator ->() { return Resource; }
	const T* operator ->() const { return Resource; }
};

struct Database;
struct DatabaseDependency : StandardResourceDependency<Database>
{
	virtual void Resolve() override
	{
		Resource = nullptr;
	}
};

struct DependencyGraph
{
	DependencyGraph(std::vector<Dependency*> dependencies)
	{
		// develop the graph 
		// all instances of a dependency should be resolved as the same dependency
		// meaning we should use handles.
	}

	// TODO the graph may be unresolvable 
	bool Resolve() {}
};


DeclareTestCategoryV3(TestsV3)
{
	DeclareTestCategoryDependencies(DatabaseDependency);

	/*
	DeclareTestV2(TestSomethingWithNoArgs)
	{
		_DatabaseDependency->Resource.whatever
	}
	*/
}




