#include <iostream>
#include <memory>
#include <list>
#include <functional>
#include <any>
#include <map>

#include <cxxabi.h>

#define private public
#include <cereal/archives/json.hpp>
#undef private

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

void get_interface(interface *inf, cereal::JSONOutputArchive &a, const type_map_t &types)
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
}

bool deserialize_arguments(arg_set_t &args, cereal::JSONInputArchive &a)
{
	std::string str;

	auto name = a.getNodeName();
	a.startNode();
	auto arr_it = a.itsIteratorStack.back();

	for(auto it = arr_it.value().Begin() ; it != arr_it.value().End() ; ++it)
	{
		auto p = it->GetInt64();
	}

	return true;
}

bool process_call(interface *inf, cereal::JSONInputArchive &in, cereal::JSONOutputArchive &out, const serializers_t &ser)
{
	if(inf == nullptr)
	{
		out(cereal::make_nvp("error", "No such object"));
		return false;
	}

	std::string name;
	in(name);

	std::string error;

	auto it = inf->func_map.find(name);
	if(it == inf->func_map.end())
	{
		out(cereal::make_nvp("error", "No such method"));
		return false;
	}

	arg_set_t args;
	deserialize_arguments(args, in);
	std::any res = it->second->call(args);

	out.setNextName("result");
	bool ser_ok = serialize(res, ser, out);
	if(ser_ok == false)
	{
		out(cereal::make_nvp("error", "Cannot serialize result"));
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

	std::stringstream ss;

	{
		cereal::JSONOutputArchive arch(ss);
		get_interface(&tes, arch, type_map_out);
		auto s = ss.str();
		std::cout << s << std::endl;
	}

	try
	{
		std::string call_json_str;

		{
			std::stringstream ss;
			{
				cereal::JSONOutputArchive call_json_arch(ss);
				call_json_arch(cereal::make_nvp("method", std::string("test")));
				call_json_arch.setNextName("arguments");
				call_json_arch.startNode();
				call_json_arch.makeArray();
				call_json_arch.finishNode();
			}
			call_json_str = ss.str();
		}

		std::cout << call_json_str << std::endl;
		std::stringstream in_ss(call_json_str);
		cereal::JSONInputArchive call_arch(in_ss);

		std::stringstream result_ss;
		cereal::JSONOutputArchive result_arch(result_ss);
		process_call(&tes, call_arch, result_arch, serializers);
	}
	catch(const cereal::Exception &e)
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
