#pragma once

#include <chrono>
#include <string>
#include <format>

namespace lsn::test_framework
{


class test_failure
{
public:
	test_failure() = default;
	test_failure(const test_failure&) = default;

	test_failure(const std::string& error, const std::string& filename, int errorLine)
	{
		_error = error;
		_filename = filename;
		_errorLine = errorLine;
	}

	std::string FormattedString() const {
		return std::format("{0} in {1}:{2}", _error, _filename, _errorLine);
	}

	friend std::ostream& operator<<(std::ostream& os, const test_failure& dt) {
		return (os << dt.FormattedString());
	}

	inline const std::string& error() const { return _error; }
	inline const std::string& filename() const { return _filename; }
	inline int linenumber() const { return _errorLine; }

private:

	std::string _error;
	std::string _filename;
	int _errorLine;
};

struct TestResult
{
	std::chrono::nanoseconds _timeStarted = std::chrono::nanoseconds::zero();
	std::chrono::nanoseconds _timeEnded = std::chrono::nanoseconds::zero();
	std::optional<test_failure> _lastFailure;

	void Reset()
	{
		_lastFailure.reset();
		_timeEnded = std::chrono::nanoseconds::zero();
		_timeStarted = std::chrono::nanoseconds::zero();
	}

	void Begin(std::chrono::nanoseconds timeStarted) {
		_timeStarted = timeStarted;
	}

	void SetFailure(const test_failure& failure)
	{
		_lastFailure = failure;
	}

	void End(std::chrono::nanoseconds timeEnded) {
		_timeEnded = timeEnded;
	}

	std::chrono::nanoseconds TimeTaken() const {
		return _timeEnded - _timeStarted;
	}

	bool HasStarted() const {
		return _timeStarted.count() > 0;
	}

	bool HasEnded() const {
		return HasRun(); 
	}

	bool HasRun() const {
		return _timeEnded.count() > 0;
	}

	bool HasPassed() const {
		return !_lastFailure.has_value();
	}

	operator bool() const { return HasPassed(); }
};

}