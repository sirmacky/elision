
#include "TestFramework/TestFramework.h"

#include <memory>

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


DeclareTestCategory(Standard)
{
	using namespace std::chrono_literals;

	DeclareTest(TestSomething,
		ValueSource(Example::GenerateSingleData),
		ValueSource(Example::GenerateSingleTupleData),
		ValueCase(42),
		ValueCase(1337),
		Arguments(int a))
	{
		AssertThat(a != 1337);
	}

	DeclareTest(TestMultiple, WithRequirement(TestConcurrency::Exclusive),
		Timeout(15ms),
		ValueSource(Example::GenerateMultipleTupleData),
		ValueCase(42, 43),
		Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	DeclareTest(TestSomethingWithNoArgs)
	{
		AssertThat(1 > 0);
	}
}


// TODO: V3
// The category should be able to specify the default concurrency of the tests
// Tests should support a timeout
 
// RESOURCES OPTION 1
// in order to speed up tests there may be test dependencies that a test may want. 
// such as loading a shared resource, or something
// the cateogry should be responsible for providing and tearing down this data
// teardown should be explicit 


#define DeclareTestCategoryV3(name) namespace name

#define DeclareTestCategoryTeardown() void Teardown()
#define DeclareTestCategorySetup() void Setup()


DeclareTestCategoryV3(TestsV3)
{
	// This will allow us to set up common start and teardown operations
	// as well as a single setup function
	DeclareTestCategorySetup() { }
	DeclareTestCategoryTeardown() { }
}

// RESOURCES OPTION 2
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
	{}
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




