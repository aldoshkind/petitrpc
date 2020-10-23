#pragma once

#include <mutex>
#include <queue>
#include <thread>
#include <condition_variable>

#include "command.h"

namespace rpc
{

class single_thread_command_executor
{
public:
	typedef std::shared_ptr<command> command_ptr;

	single_thread_command_executor(bool autostart = true)
	{
		if(autostart)
		{
			start();
		}
	}
	virtual ~single_thread_command_executor()
	{
		stop();
	}

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
		cmd_execution_thread = std::thread(&single_thread_command_executor::cmd_execution_loop, this);
	}

	void stop()
	{
		running = false;
		commands_condvar.notify_all();
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
				commands_condvar.wait(lock, [this](){return (commands.size() > 0) || (running == false);});
				if(running == false)
				{
					break;
				}
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
			catch(const std::exception &e)
			{
				cmd->error(std::string("Error running command: ") + e.what());
			}
			catch(...)
			{
				cmd->error("Error running command");
			}
		}
	}

	virtual std::string run_command(const std::string &cmd) = 0;
};

}
