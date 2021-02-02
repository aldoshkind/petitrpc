#pragma once

#include <nlohmann/json.hpp>

#include "interface_server.h"
#include "json_serialization.h"
#include "func_caller_serializer.h"
#include "rpc_exception.h"

namespace rpc::json_serialization
{

class json_interface_wrapper : public serializer
{
public:
	json_interface_wrapper()
	{
		init_types(serializers, deserializers, type_map_in, type_map_out);
	}

	void get_interface(interface_server *inf, nlohmann::json &out)
	{
		if(inf == nullptr)
		{
			return;
		}
		out["name"] = inf->name;
		out["methods"] = out.array();
		auto &methods = out["methods"];

		for(auto &func : inf->func_map)
		{
			const func_caller_base *fc = func.second.get();
			auto o = methods.object();
			serialize_caller(fc, o, type_map_out);
			methods.push_back(o);
		}
	}

	bool get_state(interface_server *inf, nlohmann::json &out) const
	{
		std::any res = inf->get_state();
		bool ser_ok = serialize(res, out);
		if(ser_ok == false)
		{
			throw(rpc::exception("Cannot serialize result"));
			return false;
		}
		return true;
	}

	bool process_call(interface_server *inf, const nlohmann::json &in, nlohmann::json &out)
	{
		if(inf == nullptr)
		{
			throw(rpc::exception("No such object"));
			return false;
		}

		std::string name = in.at("method");

		auto it = inf->func_map.find(name);
		if(it == inf->func_map.end())
		{
			throw(rpc::exception("No such method"));
			return false;
		}

		arg_set_t args;
		auto &caller = it->second;

		try
		{
			if(caller->strong_typed == true)
			{
				deserialize_arguments(args, in, caller->params);
			}
			else
			{
				deserialize_arguments(args, in);
			}
		}
		catch(const std::exception &e)
		{
			throw(rpc::exception(std::string("Cannot deserialize arguments: ") + e.what()));
		}
		catch(...)
		{
			throw(rpc::exception(std::string("Cannot deserialize arguments")));
		}

		std::any res;
		try
		{
			res = caller->call(args);
		}
		catch(const std::exception &e)
		{
			throw(rpc::exception(std::string("Method call failed: ") + e.what()));
		}
		catch(const std::string &e)
		{
			throw(rpc::exception(std::string("Method call failed: ") + e));
		}
		catch(...)
		{
			throw(rpc::exception("Method call failed"));
		}

		bool ser_ok = serialize(res, out["result"]);
		if(ser_ok == false)
		{
			throw(rpc::exception("Cannot serialize result"));
			return false;
		}

		return true;
	}

	// TODO: move to separate class
	serializers_t serializers;
	deserializers_t deserializers;
	type_map_t type_map_in;
	type_map_t type_map_out;

	bool serialize(const std::any &a, nlohmann::json &out) const override
	{
		auto ser = serializers.find(a.type().name());
		if(ser == serializers.end())
		{
			return false;
		}
		ser->second(a, out, this);
		return true;
	}

private:
	bool deserialize_arguments(arg_set_t &args, const nlohmann::json &a, const param_descr_list_t &params)
	{
		try
		{
			const nlohmann::json &arguments = a.at("arguments");

			if(arguments.size() != params.size())
			{
				throw(rpc::exception("wrong number or arguments"));
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
					throw(rpc::exception("argument " + std::to_string(i) + " deserialization error: " + e.what()));
				}
				args.push_back(a);
			}
		}
		catch(const char *e)
		{
			throw(rpc::exception(e));
		}
		catch(const std::exception &e)
		{
			throw(rpc::exception(e.what()));
		}
		catch(...)
		{
			throw(rpc::exception("Arguments deserialization error"));
		}

		return true;
	}

	bool deserialize_arguments(arg_set_t &args, const nlohmann::json &a)
	{
		try
		{
			const nlohmann::json &arguments = a.at("arguments");

			for(size_t i = 0 ; i < arguments.size() ; i += 1)
			{
				std::any a;
				auto &arg = arguments[i];
				auto type = arg.type();

				switch(type)
				{
				case nlohmann::detail::value_t::boolean:
					a = (bool)(arg);
				break;
				case nlohmann::detail::value_t::number_float:
					a = (float)(arg);
				break;
				case nlohmann::detail::value_t::number_integer:
					a = (int)(arg);
				break;
				case nlohmann::detail::value_t::number_unsigned:
					a = (unsigned)(arg);
				break;
				case nlohmann::detail::value_t::object:
					a = arg;
				break;
				case nlohmann::detail::value_t::string:
					a = (std::string)(arg);
				break;
				case nlohmann::detail::value_t::null:
					a = nullptr;
				break;
				default:
					throw(rpc::exception("argument " + std::to_string(i) + " deserialization error: not supported"));
				break;
				}

				args.push_back(a);
			}
		}
		catch(const char *e)
		{
			throw(rpc::exception(e));
		}
		catch(const std::exception &e)
		{
			throw(rpc::exception(e.what()));
		}
		catch(...)
		{
			throw(rpc::exception("Arguments deserialization error"));
		}

		return true;
	}
};

}
