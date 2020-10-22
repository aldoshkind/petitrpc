#include <iostream>
#include <memory>
#include <list>
#include <functional>
#include <any>
#include <map>

#include <cxxabi.h>

#include "interface.h"
#include "json_serialization.h"
#include "func_caller_serializer.h"

using namespace std;
using namespace rpc;
using namespace rpc::json_serialization;

std::string demangle(const char *name)
{
	int status = 0;
	return abi::__cxa_demangle(name, 0, 0, &status);
}

std::string test(int x, int y, double z)
{
	printf("%s\n", __PRETTY_FUNCTION__/*, x*/);
	std::cout << x << " " << y << " " <<  z << std::endl;
	return "255";
}

class testo : public interface
{
public:
	testo()
	{
		name = "testo";
		register_method("test", (std::function<void(const std::string &)>)std::bind(&testo::test, this, std::placeholders::_1), {"str"});
		register_method("testb", (std::function<void(const std::string &, int)>)std::bind(&testo::testb, this, std::placeholders::_1, std::placeholders::_2), {"str", "a"});
		register_method("testc", (std::function<void(const std::string &, float, float)>)std::bind(&testo::testc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), {"str", "a", "b"});
	}

	void test(const std::string &s)
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, s.c_str());
	}

	void testb(const std::string &s, int x)
	{
		printf("%s: %s %d\n", __PRETTY_FUNCTION__, s.c_str(), x);
	}

	void testc(const std::string &s, float a, float b)
	{
		printf("%s: %s %f %f\n", __PRETTY_FUNCTION__, s.c_str(), a, b);
	}
};

/*void get_interface(interface *inf, cereal::JSONOutputArchive &a, const type_map_t &types)
{
	if(inf == nullptr)
	{
		return;
	}
	a(cereal::make_nvp("name", inf->name));
	a.setNextName("methods");
	a.startNode();
	a.makeArray();

	for(auto &func : inf->func_map)
	{
		a.startNode();
		const func_caller_base *fc = func.second.get();
		serialize(fc, a, types);
		a.finishNode();
	}

	a.finishNode();
}*/

bool deserialize_arguments(arg_set_t &args, nlohmann::json &a, const param_descr_list_t &params, const deserializers_t &desers)
{
	std::string str;

	if(a.contains("arguments") == false)
	{
		return false;
	}

	nlohmann::json &arguments = a["arguments"];

	if(arguments.size() != params.size())
	{
		return false;
	}

	for(size_t i = 0 ; i < arguments.size() ; i += 1)
	{
		std::any a;
		deserialize(arguments.at(i), desers, a, std::next(params.begin(), i)->first);
		args.push_back(a);
	}

	return true;
}

bool process_call(interface *inf, nlohmann::json &in, nlohmann::json &out, const serializers_t &ser, const deserializers_t &desers)
{
	if(inf == nullptr)
	{
		out["error"] = std::string("No such object");
		return false;
	}

	std::string name = in["method"];

	std::string error;

	auto it = inf->func_map.find(name);
	if(it == inf->func_map.end())
	{
		out["error"] = std::string("No such method");
		return false;
	}

	arg_set_t args;
	bool deser_ok = deserialize_arguments(args, in, it->second->params, desers);
	if(deser_ok == false)
	{
		out["error"] = std::string("Cannot deserialize arguments");
		return false;
	}

	std::any res = it->second->call(args);

	bool ser_ok = serialize(res, ser, out["result"]);
	if(ser_ok == false)
	{
		out["error"] = std::string("Cannot serialize result");
		return false;
	}

	return true;
}


int main()
{
	serializers_t serializers;
	deserializers_t deserializers;
	type_map_t type_map_in;
	type_map_t type_map_out;

	init_types(serializers, deserializers, type_map_in, type_map_out);

	testo tes;

	{
		/*cereal::JSONOutputArchive arch(ss);
		get_interface(&tes, arch, type_map_out);
		auto s = ss.str();
		std::cout << s << std::endl;*/
	}

	try
	{
		nlohmann::json call_json = {{"method", "test"}, {"arguments", {"test", }}};
		std::cout << call_json.dump() << std::endl;
		nlohmann::json result;
		process_call(&tes, call_json, result, serializers, deserializers);
		std::cout << result.dump() << std::endl;;
	}
	catch(const std::exception &e)
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, e.what());
	}
	catch(const char *e)
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, e);
	}
	catch (...)
	{
		printf("%s: exception\n", __PRETTY_FUNCTION__);
	}
	return 0;
}
