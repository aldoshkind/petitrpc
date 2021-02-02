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
		try
		{
			deserialize_arguments(args, in, it->second->params);
		}
		catch(const std::exception &e)
		{
			throw(rpc::exception(std::string("Cannot deserialize arguments: ") + e.what()));
		}
		catch(...)
		{
			throw(rpc::exception(std::string("Cannot deserialize arguments")));
		}

		std::any res = it->second->call(args);

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
};

}
