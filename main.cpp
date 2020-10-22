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

#include "interface.h"
#include "json_serialization.h"
#include "func_caller_serializer.h"
#include "json_interface_wrapper.h"

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
		register_method("testc", (std::function<int(const std::string &, float, float)>)std::bind(&testo::testc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), {"str", "a", "b"});
	}

	void test(const std::string &s)
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, s.c_str());
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

class command
{
public:
	command() = default;
	virtual ~command() = default;

	virtual std::string get_cmd() const = 0;
	virtual void reply(const std::string &cmd) = 0;
	virtual void error(const std::string &err) = 0;
};

class command_executor
{
public:
	typedef std::shared_ptr<command> command_ptr;

	command_executor()
	{
		start();
	}
	virtual ~command_executor() = default;

	bool add_command(command_ptr cmd)
	{
		std::unique_lock<decltype (commands_mutex)> lock(commands_mutex);
		commands.push(cmd);
		commands_condvar.notify_all();
		return true;
	}

private:
	typedef std::queue<command_ptr> commands_t;
	commands_t commands;
	std::mutex commands_mutex;
	std::condition_variable commands_condvar;
	std::thread cmd_execution_thread;
	bool running = false;

	void start()
	{
		stop();
		running = true;
		cmd_execution_thread = std::thread(&command_executor::cmd_execution_loop, this);
	}

	void stop()
	{
		running = false;
		if(cmd_execution_thread.joinable())
		{
			cmd_execution_thread.join();
		}
	}

	void cmd_execution_loop()
	{
		for( ; running ; )
		{
			command_ptr cmd;
			{
				std::unique_lock<decltype (commands_mutex)> lock(commands_mutex);
				commands_condvar.wait(lock, [this](){return commands.size() > 0;});
				cmd = commands.front();
				commands.pop();
			}
			try
			{
				cmd->reply(run_command(cmd->get_cmd()));
			}
			catch(const char *e)
			{
				cmd->error(std::string("Error running command: ") + e);
			}
			catch(const exception &e)
			{
				cmd->error(std::string("Error running command: ") + e.what());
			}
			catch(...)
			{
				cmd->error("Error running command");
			}
		}
	}

	virtual std::string run_command(const std::string &cmd)
	{
		printf("%s: command: %s\n", __PRETTY_FUNCTION__, cmd.c_str());
		return "ok";
	}
};

class interface_executor : public command_executor
{
public:
	interface_executor()
	{
		//
	}

	~interface_executor() = default;

	void set_interface(interface *i)
	{
		iface = i;
	}

private:
	std::string run_command(const std::string &cmd)
	{
		nlohmann::json json;
		nlohmann::json request;
		try
		{
			json = nlohmann::json::parse(cmd);
			request = json.at("request");
		}
		catch(const char *e)
		{
			return "Error parsing request";
		}
		catch(const exception &e)
		{
			return "Error parsing request";
		}
		catch(...)
		{
			return "Error parsing request";
		}

		if(iface == nullptr)
		{
			return "Interface not set";
		}
		nlohmann::json out;
		wrapper.process_call(iface, json, out);
		return out.dump();
	}

	interface *iface = nullptr;
	json_interface_wrapper wrapper;
};


int main()
{
	testo tes;

	interface_executor ex;
	ex.set_interface(&tes);

	json_interface_wrapper wrapper;

	{
		nlohmann::json interf;
		wrapper.get_interface(&tes, interf);
		std::cout << interf.dump(1, '\t') << std::endl;
	}

	try
	{
		nlohmann::json call_json = {{"method", "test"}, {"arguments", {"test", }}};
		std::cout << call_json.dump(1, '\t') << std::endl;
		nlohmann::json result;
		wrapper.process_call(&tes, call_json, result);
		std::cout << result.dump(1, '\t') << std::endl;
		
		call_json = {{"method", "testc"}, {"arguments", {"test", 13, 14}}};
		std::cout << call_json.dump(1, '\t') << std::endl;
		wrapper.process_call(&tes, call_json, result);
		std::cout << result.dump(1, '\t') << std::endl;		
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
