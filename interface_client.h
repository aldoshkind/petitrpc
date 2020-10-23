#pragma once

#include <any>
#include <tuple>
#include <string>
#include <vector>

namespace rpc
{

class call_forwarder
{
public:
	virtual ~call_forwarder() = default;
	virtual std::any forward_call(const std::string method, const std::vector<std::any> &args) = 0;
};

template <int i = 0, class ... Ts>
typename std::enable_if<i == sizeof...(Ts), void>::type
pack(const std::tuple<Ts ...> &, const std::vector<std::any> &)
{
	return;
}

template <int i = 0, class ... Ts>
typename std::enable_if<i < sizeof...(Ts), void>::type
pack(const std::tuple<Ts ...> &t, std::vector<std::any> &v)
{
	try
	{
		v.push_back(std::get<i>(t));
	}
	catch(const std::bad_any_cast &e)
	{
		throw "Incompatible types";
	}
	pack<i + 1>(t, v);
}

class interface_client
{
public:
	interface_client()
	{
		//
	}
	virtual ~interface_client()
	{
		//
	}

	template <class R, class ...Ts>
	R call(const std::string &method, Ts ... arguments)
	{
		using tup = std::tuple<typename std::decay<Ts>::type...>;
		tup args(arguments ...);
		std::vector<std::any> arg_vec;
		pack(args, arg_vec);
		
		std::vector<std::string> vec;
		vec.insert(vec.end(), {typeid(Ts).name() ...});

		for(const auto &a : arg_vec)
		{
			printf("%s\n", a.type().name());
		}

		if(fwd != nullptr)
		{
			try
			{
				auto res = fwd->forward_call(method, arg_vec);
				return std::any_cast<R>(res);
			}
			catch(...)
			{
				return R();
			}				
		}
		else
		{
			return R();
		}
	}
	
	call_forwarder *fwd = nullptr;
};

}
