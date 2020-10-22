#pragma once

#include <stdio.h>

#include <map>
#include <memory>
#include <typeinfo>

#include "func_caller.h"

namespace rpc
{

class interface
{
public:
	interface()
	{
		//
	}

	virtual ~interface() = default;

	template <typename R, class ...Ts>
	void register_method(std::string name, const std::function<R(Ts...)> &f, std::array<std::string, sizeof...(Ts)> parameter_names)
	{
		auto n = new func_caller(f);
		func_ptr fc = func_ptr(n);
		fc->name = name;
		func_map[name] = fc;
		for(size_t i = 0 ; i < parameter_names.size() ; i += 1)
		{
			fc->set_param_name(i, parameter_names[i]);
		}
	}

	typedef std::shared_ptr<func_caller_base> func_ptr;
	typedef std::map<std::string, func_ptr> func_map_t;
	func_map_t func_map;

	std::string name;
};

}
