elision is a multithreaded test framework aimed at making writing and controlling tests simple and natural

```
DeclareTestCategory(Examples)
{
	using namespace std::chrono_literals;

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
		ValueSource(Example::ValueSources::IntegerRange<1, 5>),
		ValueSource(Example::ValueSources::SingleValueTupleRange<6, 10>))
	{
		AssertThat(a < 1337);
	}

	// Multiple arguments can also be supported via valuecase's and tuple based value sources
	// Note that the order of option arguments to the DeclareTest macro is irrelevant
	// so tests can be formatted more organically like below
	DeclareTest(MultipleArguments,
		ValueCase(42, 43),
		ValueCase(44, 45),
		ValueSource(Example::ValueSources::MultipleValueTupleData<1, 5>),
		ValueSource(Example::ValueSources::MultipleValueTupleData<6, 10>),
		Arguments(int a, int b))
	{
		AssertThat(a < b);
	}

	// Additional test options are also availble
	DeclareTest(Options,
		/* Configurable concurrency requirements allow tests to be run
		 * Exclusive: can be the only test running
		 * Privileged: can only be run on the privelaged job thread
		 * Multithreaded (default): Can run on any testing job thread
	   //*/
		WithConcurrency(TestConcurrency::Exclusive),
		// Tests are automatically stopped and failed if they exceed the timeout
		Timeout(15ms))
	{
		while (true) {}
	}
}
```
