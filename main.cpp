#include <iostream>
#include <memory>
#include <list>
#include <functional>
#include <any>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include <cxxabi.h>

#include "interface_server.h"
#include "json_serialization.h"
#include "func_caller_serializer.h"
#include "json_interface_wrapper.h"
#include "command_executor.h"
#include "interface_client.h"

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

class testo : public interface_server
{
public:
	testo()
	{
		name = "testo";
		register_method("test", (std::function<int(const std::string &)>)std::bind(&testo::test, this, std::placeholders::_1), {"str"});
		register_method("testb", (std::function<void(const std::string &, int)>)std::bind(&testo::testb, this, std::placeholders::_1, std::placeholders::_2), {"str", "a"});
		register_method("testc", (std::function<int(const std::string &, float, float)>)std::bind(&testo::testc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), {"str", "a", "b"});
	}

	int test(const std::string &s)
	{
		return printf("%s: %s\n", __PRETTY_FUNCTION__, s.c_str());
	}

	void testb(const std::string &s, int x)
	{
		printf("%s: %s %d\n", __PRETTY_FUNCTION__, s.c_str(), x);
	}

	int testc(const std::string &s, float a, float b)
	{
		return printf("%s: %s %f %f\n", __PRETTY_FUNCTION__, s.c_str(), a, b);
	}
};





class interface_executor : public single_thread_command_executor
{
public:
	interface_executor()
	{
		//
	}

	~interface_executor() = default;

	void set_interface(interface_server *i)
	{
		iface = i;
	}

private:
	void throw_if_interface_not_set()
	{
		if(iface == nullptr)
		{
			throw rpc::exception("interface not set");
		}
	}

	std::string run_command(const std::string &cmd)
	{
		try
		{
			nlohmann::json json = nlohmann::json::parse(cmd);
			std::string action = json.at("action");
			nlohmann::json out;

			if(action == "call")
			{
				throw_if_interface_not_set();
				const nlohmann::json &body = json.at("body");
				wrapper.process_call(iface, body, out);
			}

			if(action == "get_interface")
			{
				throw_if_interface_not_set();
				wrapper.get_interface(iface, out);
			}
			
			return out.dump();
		}
		catch(const char *e)
		{
			throw rpc::exception(e);
		}
		catch(const std::exception &e)
		{
			throw rpc::exception(e.what());
		}
		catch(...)
		{
			throw rpc::exception("error while parsing request json");
		}
	}

	interface_server *iface = nullptr;
	json_interface_wrapper wrapper;
};


class cmd : public command
{
public:
	cmd(const std::string &command) : c(command)
	{
		//
	}
	
	std::string get_cmd() const override
	{
		return c;
	}
	
	void reply(const std::string &result) override
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, nlohmann::json::parse(result).dump(1, '\t').c_str());
	}
	
	void error(const std::string &err) override
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, err.c_str());
	}

private:
	std::string c;
};



typedef std::vector<uint8_t> buf_t;

class channel_writer
{
public:
	channel_writer(){}
	virtual ~channel_writer(){}
	
	virtual void send(const buf_t &buf) = 0;
};

class channel_client : public channel_writer
{
public:
	virtual ~channel_client() = default;
	
	virtual void send(const buf_t &request, buf_t &reply) = 0;
	virtual void send(const buf_t &buf) override = 0;
};

class channel_server// : public channel_writer
{
public:
	typedef std::shared_ptr<channel_server> this_ref_t;
	
	class message
	{
	public:
		message(channel_server::this_ref_t ref)
		{
			master = std::weak_ptr(ref);
		}
		
		void reply(const buf_t &reply)
		{
			auto sp = master.lock();
			if(sp)
			{
				sp->reply(this, reply);
			}
			printf("%s: %d %s\n", __PRETTY_FUNCTION__, (int)reply.size(), sp ? "ok" : "false");
		}
		
		std::weak_ptr<channel_server> master;
	};	
	
	channel_server()
	{
		this_ref = this_ref_t(this, [](channel_server *){});
	}
	
	typedef std::shared_ptr<message> message_ptr;
	
	virtual ~channel_server() = default;
	
	//virtual void reply(message_ptr m, const buf_t &reply) = 0;
	virtual void reply(message *m, const buf_t &reply)
	{
		printf("%s\n", __PRETTY_FUNCTION__);
	}
	
//private:
	this_ref_t this_ref;
};



class json_forwarder : public call_forwarder
{
public:
	json_forwarder()
	{
		init_types(serializers, deserializers, type_map_in, type_map_out);
	}
	
	std::any forward_call(const std::string method, const std::vector<std::any> &args) override
	{
		nlohmann::json call;
		call["method"] = method;
		auto &arg_node = call["arguments"] = nlohmann::json::array({});
		
		for(const auto &arg : args)
		{
			nlohmann::json arg_json;
			serialize(arg, serializers, arg_json);
			arg_node.push_back(arg_json);
		}
		
		printf("%s\n", call.dump(1, '\t').c_str());
		
		return std::any();
	}
	
private:	
	serializers_t serializers;
	deserializers_t deserializers;
	type_map_t type_map_in;
	type_map_t type_map_out;
};





int main()
{
	testo tes;

	interface_executor ex;
	ex.set_interface(&tes);

	interface_client ic;
	json_forwarder jf;
	ic.fwd = &jf;

	ic.call<int, std::string, int>("lol", "kek", 17);
	
	
	
	channel_server *cs = new channel_server;
	auto m = std::make_shared<channel_server::message>(cs->this_ref);
	//delete cs;
	m->reply(buf_t());
	
	
	

	ex.add_command(std::make_shared<cmd>(R"({"action": "call", "body": {"method":"test", "arguments": ["15"]}})"));
	ex.add_command(std::make_shared<cmd>(R"({"action": "get_interface"})"));
	ex.add_command(std::make_shared<cmd>(R"({"action": "call", "body": {"method":"testb", "arguments": ["15"]}})"));
	ex.add_command(std::make_shared<cmd>(R"({"action": "call", "body": {"method":"testc", "arguments": ["15", 16, 18]}})"));

	json_interface_wrapper wrapper;

	{
		nlohmann::json interf;
		wrapper.get_interface(&tes, interf);
		//std::cout << interf.dump(1, '\t') << std::endl;
	}

	try
	{
		nlohmann::json call_json = {{"method", "test"}, {"arguments", {"test", }}};
		//std::cout << call_json.dump(1, '\t') << std::endl;
		nlohmann::json result;
		//wrapper.process_call(&tes, call_json, result);
		//std::cout << result.dump(1, '\t') << std::endl;
		
		call_json = {{"method", "testc"}, {"arguments", {"test", 13, 14}}};
		//std::cout << call_json.dump(1, '\t') << std::endl;
		//wrapper.process_call(&tes, call_json, result);
		//std::cout << result.dump(1, '\t') << std::endl;		
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
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	return 0;
}
