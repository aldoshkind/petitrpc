#pragma once

#include <nlohmann/json.hpp>

#include "interface.h"
#include "json_serialization.h"

namespace rpc::json_serialization
{

class json_interface_wrapper_exception : public std::exception
{
public:
	json_interface_wrapper_exception(const std::string &message) : msg(message)
	{
		//
	}
	
	const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
	{
		return msg.c_str();
	}
	
private:
	std::string msg;
};

class json_interface_wrapper
{
public:
	json_interface_wrapper()
	{
		init_types(serializers, deserializers, type_map_in, type_map_out);
	}

	void get_interface(interface *inf, nlohmann::json &a)
	{
		if(inf == nullptr)
		{
			return;
		}
		a["name"] = inf->name;
		a["methods"] = a.array();
		auto &methods = a["methods"];

		for(auto &func : inf->func_map)
		{
			const func_caller_base *fc = func.second.get();
			auto o = methods.object();
			serialize(fc, o, type_map_out);
			methods.push_back(o);
		}
	}

	bool process_call(interface *inf, nlohmann::json &in, nlohmann::json &out)
	{
#warning завернуть всю работу с json в try catch
		
		if(inf == nullptr)
		{
			throw(json_interface_wrapper_exception("No such object"));
			return false;
		}

		std::string name = in["method"];

		auto it = inf->func_map.find(name);
		if(it == inf->func_map.end())
		{
			throw(json_interface_wrapper_exception("No such method"));
			return false;
		}

		arg_set_t args;
		std::string error;
		bool deser_ok = deserialize_arguments(args, in, it->second->params, error);
		if(deser_ok == false)
		{
			throw(json_interface_wrapper_exception(std::string("Cannot deserialize arguments: ") + error));
			return false;
		}

		std::any res = it->second->call(args);

		bool ser_ok = serialize(res, serializers, out["result"]);
		if(ser_ok == false)
		{
			throw(json_interface_wrapper_exception("Cannot serialize result"));
			return false;
		}

		return true;
	}

private:
	serializers_t serializers;
	deserializers_t deserializers;
	type_map_t type_map_in;
	type_map_t type_map_out;

	bool deserialize_arguments(arg_set_t &args, nlohmann::json &a, const param_descr_list_t &params, std::string &error)
	{
#warning завернуть всю работу с json в try catch		

		std::string str;

		if(a.contains("arguments") == false)
		{
			error = "no 'arguments' found";
			return false;
		}

		nlohmann::json &arguments = a["arguments"];

		if(arguments.size() != params.size())
		{
			error = "wrong number or arguments";
			return false;
		}

		for(size_t i = 0 ; i < arguments.size() ; i += 1)
		{
			std::any a;
			try
			{
				deserialize(arguments.at(i), deserializers, a, std::next(params.begin(), i)->first);
			}
			catch(const std::exception &e)
			{
				error = "argument " + std::to_string(i) + " deserialization error: " + e.what();
				return false;
			}
			args.push_back(a);
		}

		return true;
	}
};

}
