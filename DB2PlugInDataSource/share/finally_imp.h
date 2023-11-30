#pragma once

template <typename F>
struct finally_action
{
	finally_action(F f) : clean(f) {}
	~finally_action() { clean(); }
	F clean;
};

template <class F>
finally_action<F> finally(F f)
{
	return finally_action<F>(f);
}