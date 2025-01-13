#include "TestFramework/TestFramework.h"

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
	std::vector<int> IntegerRange()
	{
		constexpr int diff = (long)Start - (long)End;
		constexpr int direction = MathUtils::SignOf(diff);
		const int num = std::abs(diff);

		std::vector<int> values(num);
		for (int i = 0; i < num; ++i)
			values[i] = Start + (i * direction);

		return values;
	}
}

DeclareTestCategory(TestCoordinator)
{
	using namespace std::chrono_literals;

	DeclareTest(ExclusiveSleeps,  WithRequirement(TestConcurrency::Exclusive), 
		ValueSource(Tests::ValueSources::IntegerRange<1,20>),
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

	DeclareTest(InfiniteLoading)
	{
		while (true) {}
	}
}