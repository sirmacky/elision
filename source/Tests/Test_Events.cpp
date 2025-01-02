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