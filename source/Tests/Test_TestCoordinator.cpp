#include "TestFramework/TestRunner.h"

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

DeclareTestCategoryV2(TestCoordinator)
{
	using namespace std::chrono_literals;

	DeclareTestV2(ExclusiveSleeps, WithRequirement(TestConcurrency::Exclusive),
		ValueSource(Tests::ValueSources::IntegerRange<1,20>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTestV2(PrivelagedSleeps, WithRequirement(TestConcurrency::Privelaged),
		ValueSource(Tests::ValueSources::IntegerRange<1, 20>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTestV2(AnySleeps, WithRequirement(TestConcurrency::Any),
		ValueSource(Tests::ValueSources::IntegerRange<1, 100>),
		Arguments(int _))
	{
		std::this_thread::sleep_for(1s);
	}

	DeclareTestV2(InititeLoading)
	{
		while (true) {}
	}
}