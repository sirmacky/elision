
#include "TestFramework/TestFramework.h"
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>

namespace MathUtils
{
	constexpr auto SignOf(auto val)
	{
		if (val < 0)
			return -1;
		if (val > 0)
			return 1;
		return 0;
	}
}

// TODO: Move to it's own file
namespace Tests::ValueSources
{
	template<int Start, int End>
	auto IntegerRange()
	{
		constexpr int diff = (long)End - (long)Start;
		constexpr int direction = MathUtils::SignOf(diff);
		const int num = std::abs(diff) + 1;

		std::vector<int> values(num);
		for (int i = 0; i < num; ++i)
			values[i] = Start + (i * direction);

		return values;
	}

	template<int Start, int End>
	auto SingleValueTupleRange()
	{
		constexpr int diff = (long)End - (long)Start;
		constexpr int direction = MathUtils::SignOf(diff);
		const int num = std::abs(diff) + 1;

		std::vector<std::tuple<int>> a(num);
		for (int i = 0; i < num; ++i)
			a[i] = (std::make_tuple(Start + (i * direction)));
		return a;
	}

	template<int Start, int End>
	auto MultipleValueTupleData()
	{
		constexpr int diff = (long)End - (long)Start;
		constexpr int direction = MathUtils::SignOf(diff);
		const int num = std::abs(diff) + 1;

		std::vector<std::tuple<int, int>> a(num);
		for (int i = 0; i < num; ++i)
		{
			auto val = Start + (i * direction);
			a[i] = (std::make_tuple(val, val * 10));
		}
			
		return a;
	}
}

DeclareTestCategory(FrameworkStructure)
{
	using namespace std::chrono_literals;

	DeclareTest(TestWithNoArgs) 
	{}

	DeclareTest(TestWithTimeout, Timeout(15ms))
	{}

	DeclareTest(TestWithThreadPriority, WithRequirement(TestConcurrency::Exclusive))
	{}

	DeclareTest(TestValueSources, 
		ValueSource(Tests::ValueSources::MultipleValueTupleData<1, 10>),
		ValueCase(42, 43),
		Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	DeclareTest(TestMultipleValueSources,
		ValueSource(Tests::ValueSources::IntegerRange<1,10>),
		ValueSource(Tests::ValueSources::SingleValueTupleRange<1,10>),
		ValueCase(42),
		ValueCase(1337),
		Arguments(int a))
	{
		AssertThat(a != 1337);
	}
}

DeclareTestCategory(FrameworkConcurrency)
{
	using namespace std::chrono_literals;

	DeclareTest(ExclusiveSleeps, WithRequirement(TestConcurrency::Exclusive),
		ValueSource(Tests::ValueSources::IntegerRange<1, 20>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTest(PrivelagedSleeps, WithRequirement(TestConcurrency::Privelaged),
		ValueSource(Tests::ValueSources::IntegerRange<1, 20>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTest(AnySleeps, WithRequirement(TestConcurrency::Any),
		ValueSource(Tests::ValueSources::IntegerRange<1, 100>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTest(SleepingTimeout, Timeout(1s))
	{
		std::this_thread::sleep_for(5s);
	}

	DeclareTest(InfiniteLoop, Timeout(1s))
	{
		while (true) {}
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

#define DeclareTestCategorySetup() void Setup()
#define DeclareTestCategoryTeardown() void Teardown()


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



// All tests must belong to a category.
DeclareTestCategory(Examples)
{
	using namespace std::chrono_literals;

	// These macros allow the test category to set up and teardown additional data 
	// that all tests in the category may want to use. This will be supported later
	// with a dependency graph based system allowing multiple categories to share
	// resources, speeding up test runs
	DeclareTestCategorySetup() {}
	DeclareTestCategoryTeardown() {}


	// Simple tests can be declared with no arguments
	DeclareTest(NoArguments)
	{
		AssertThat(true != false);
	}

	// An argument can optionally be defined
	DeclareTest(SingleArgument, Arguments(int a),
		// Which can either come from explicit pre-defined value cases
		ValueCase(42),
		ValueCase(44),
		// Or from value sources as either a vector<T> or vector<tuple<T>>
		// You can mix and match multiple sources, and value cases together simultaneously
		ValueSource(Tests::ValueSources::IntegerRange<1, 5>),
		ValueSource(Tests::ValueSources::SingleValueTupleRange<6, 10>))
	{
		AssertThat(a < 1337);
	}

	// Multiple arguments can also be supported via valuecase's and tuple based value sources
	// Note that the order of option arguments to the DeclareTest macro is irrelevant
	// so tests can be formatted more organically like below
	DeclareTest(MultipleArguments,
		ValueCase(42, 43),
		ValueCase(44, 45),
		ValueSource(Tests::ValueSources::MultipleValueTupleData<1, 5>),
		ValueSource(Tests::ValueSources::MultipleValueTupleData<6, 10>),
		Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	// Additional test options are also availble
	DeclareTest(Options,
		/* Configurable concurrency requirements allow tests to be run
		 * exclusively: can be the only test running
		 * privelaged: can only be run on the privelaged job thread
		 * multithreaded (default): Can run on any testing job thread
	   //*/
		WithRequirement(TestConcurrency::Exclusive),
		// Tests are automatically stopped and failed if they exceed the timeout
		Timeout(15ms))
	{
		while (true) {}
	}
}




