#pragma once



// Warn when we're attaching to the same function if possible
// Warn when we're causing re-entrance (allow? disallow?)
// 
// Support uniform attach / detach with std::function, member functions, static functions 
// Support transient subscription
// Support sticky events
// Support detach when firing the event
// automatic cleanup if anything dies.

// things should fire in the order they were subscribed.

#include <vector>
#include <map>
#include <functional>
#include <utility>


namespace HashUtils 
{
	constexpr unsigned int perfect_hash(unsigned int x) {
		x = ((x >> 16) ^ x) * 0x45d9f3b;
		x = ((x >> 16) ^ x) * 0x45d9f3b;
		x = (x >> 16) ^ x;
		return x;
	}

	template <typename Object, typename ReturnType, typename... Args>
	std::function<ReturnType(Args...)> easy_bind(ReturnType(Object::* MemPtr)(Args...), Object* obj)
	{
		return [=](Args... args) mutable -> ReturnType { return ((*obj).*MemPtr)(args...); };
	}

	template <typename Object, typename ReturnType, typename... Args>
	std::function<void(Args...)> easy_bind_to_void(ReturnType(Object::* MemPtr)(Args...), Object* obj)
	{
		return [=](Args... args) mutable -> void { ((*obj).*MemPtr)(args...); };
	}
}

enum class EventId : unsigned int
{
	Invalid = 0,
};

class EventSubscriptionHandle
{

	std::function<void()> _detach = nullptr;

public:

	EventSubscriptionHandle() = default;
	EventSubscriptionHandle(EventSubscriptionHandle& other) = delete;
	EventSubscriptionHandle(EventSubscriptionHandle&& other)
	{
		this->_detach = other._detach;
		other._detach = nullptr;
	}
	
	~EventSubscriptionHandle()
	{
		Reset();
	}

	void Reset()
	{
		if (_detach)
		{
			std::invoke(_detach);
			_detach = nullptr;
		}
	}

	template<typename T>
	void Set(T& ev, EventId id)
	{
		Reset();
		_detach = [id, &ev]() { ev.Detach(id); };
	}

	template<typename T, typename I, typename R, typename...Args>
	void Attach(T& ev, I* instance, R(T::*func_ptr) (Args...))
	{
		Set(ev, ev.Attach(instance, func_ptr));
	}

	template<typename T, typename R, typename...Args>
	void Attach(T& ev, std::function<R(Args...)> func)
	{
		Set(ev, ev.Attach(func));
	}
};



template <typename Callable>
struct OrderedEventStorage
{
	using callable_t = Callable;
private:
	unsigned int _id{ 0 };
	[[nodiscard]] EventId next_id() {
		return static_cast<EventId>(++_id);
	}

	std::vector<std::pair<EventId, callable_t>> _callables{};
public:

	[[nodiscard]] EventId Insert(callable_t func)
	{
		EventId id = next_id();
		_callables.emplace(std::make_pair(id, func));
		return id;
	}

	bool Remove(EventId id) 
	{
		for (auto iter = _callables.begin(); iter != _callables.end(); ++iter)
		{
			if (iter->first == id)
			{
				_callables.erase(iter);
				return true;
			}
		}
		return false;
	}

	template<typename...Args>
	void Dispatch(Args&...args)
	{
		for (const auto callable : _callables) {
			callable.second(args...);
		}
	}
};

template<typename Callable>
struct UnorderedEventStorage
{
	using callable_t = Callable;
private:
	
	unsigned int _id{ 0 };
	std::map<EventId, callable_t> _callables{};
	

	[[nodiscard]] EventId next_id() {
		return static_cast<EventId>(HashUtils::perfect_hash(++_id));
	}

public:

	bool Remove(EventId id) {
		return _callables.erase(id);
	}

	[[nodiscard]] EventId Insert(callable_t func)
	{
		// TODO: This method does not allow the callbacks to come back in subscription order
		EventId id = next_id();
		_callables.emplace(id, func);
		return id;
	}

	template<typename...Args>
	void Dispatch(Args&...args)
	{
		for (const auto [_, callable] : _callables)
			callable(args...);
	}
};


template<typename Storage, typename...Args>
class SEvent
{
	Storage _storage;

public:
	template<typename T, typename R>
	using FuncPtr = R(T::*) (Args...);

	using callabale = Storage::callable_t;


	template<typename T, typename R>
	[[nodiscard]] EventId Attach(T* instance, FuncPtr<T, R> func)
	{
		return Attach(HashUtils::easy_bind_to_void(func, instance));
	}

	[[nodiscard]] EventId Attach(callabale func)
	{
		if (func)
			return _storage.Insert(func);
		return EventId::Invalid;
	}

	bool DetachAndPreserveId(EventId& id)
	{
		return _storage.Remove(id);
	}

	bool Detach(EventId& id)
	{
		bool ret = DetachAndPreserveId(id);
		id = EventId::Invalid;
		return ret;
	}

	void Dispatch(Args&... args)
	{
		// TODO: Non-rentrant
		// TODO: Protections from being mutated
		_storage.Dispatch(args...);
	}
};

template<typename...Args> using OrderedEvent = SEvent<OrderedEventStorage<std::function<void(Args...)>>, Args...>;
template<typename...Args> using UnorderedEvent = SEvent<UnorderedEventStorage<std::function<void(Args...)>>, Args...>;


template<typename T> 
class ObservableValue : public OrderedEvent<const T&, const T&>
{
	T value{ };

public:
	using OrderedEvent<const T&, const T&>::Dispatch;

	operator const T& () const { return value; }
	ObservableValue<T>& operator=(const T& other)
	{
		SetValue(other);
		return *this;
	}

	T GetValue() const { return value; }
	void SetValue(const T& val)
	{
		T orig = value;
		value = val;
		Dispatch(orig, val);
	}
};


// TODO:
template<typename...Args> using StickyEvent = int;

/*
namespace Exploration
{

	OrderedEvent<> event{};
	EventId eventId;

	void Initiatlize()
	{
		EventSubscriptionHandle handle;
		// handle.Attach(event, &OnEvent);

		handle.Attach(event, this, &OnEvent);
		
		// handle.Attach(event, [](){ });
		
		// this sucks, required the event to understand the handle
		// event.Attach(handle, &OnEvent);
		// though we need the 

		// this doesnt work
		// handle = event.Attach(&OnEvent);

		// this works
		eventId = event.Attach(&OnEvent);
	}

	void Shutdown()
	{
		event.Detach(eventId);
	}

	void OnEvent() {};
	}
*/
