#pragma once

#include <functional>
#include <any>

#include "func_caller_base.h"

namespace rpc
{

template <int i = 0, class ... Ts>
typename std::enable_if<i == sizeof...(Ts), void>::type
copy(const std::tuple<Ts ...> &, const std::vector<std::any> &)
{
	return;
}

template <int i = 0, class ... Ts>
typename std::enable_if<i < sizeof...(Ts), void>::type
copy(std::tuple<Ts ...> &t, const std::vector<std::any> &v)
{
	using type = typename std::remove_reference_t<decltype(std::get<i>(t))>;
	try
	{
		std::get<i>(t) = std::any_cast<type>(v[i]);
	}
	catch(const std::bad_any_cast &e)
	{
		throw "Incompatible types";
	}
	copy<i + 1>(t, v);
}

template <int i = 0, class ... Ts_params>
typename std::enable_if<i == sizeof...(Ts_params), void>::type
add_param(const param_descr_list_t &)
{
	return;
}

template <int i = 0, class ... Ts_params>
typename std::enable_if<i < sizeof...(Ts_params), void>::type
add_param(param_descr_list_t &v)
{
	v.push_back({typeid(typename std::tuple_element<i, std::tuple<Ts_params...>>::type).name(), ""});
	add_param<i + 1, Ts_params ...>(v);
}

template <class R, class ... Ts>
class func_caller : public func_caller_base
{
public:
	func_caller(const std::function<R(Ts...)> &f) : func(f)
	{
		init_args();
		return_type = typeid (R).name();
	}

	void init_args()
	{
		add_param<0, Ts ...>(params);
	}

	std::any call(const arg_set_t &a)
	{
		return call_<R>(a);
	}


private:
	std::function<R(Ts...)> func;

	template <class RR>
	typename std::enable_if<std::is_same<RR, void>::value, std::any>::type
	call_(const arg_set_t &a)
	{
		using tup = std::tuple<typename std::decay<Ts>::type...>;
		tup args;

		if(std::tuple_size<tup>::value != a.size())
		{
			throw "Wrong count of parameters";
			return std::any();
		}

		copy(args, a);

		std::apply(func, args);
		return std::any();
	}


	template <class RR>
	typename std::enable_if<!std::is_same<RR, void>::value, std::any>::type
	call_(const arg_set_t &a)
	{
		using tup = std::tuple<typename std::decay<Ts>::type...>;
		tup args;

		if(std::tuple_size<tup>::value != a.size())
		{
			throw "Wrong count of parameters";
			return R();
		}

		copy(args, a);

		return std::apply(func, args);
	}
};

}
