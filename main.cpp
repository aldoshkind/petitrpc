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





class interface_executor : public single_thread_command_executor, public json_interface_wrapper
{
public:
	interface_executor()
	{
		//
	}

	~interface_executor() = default;

	void set_interface(interface_server *i)
	{
		server = i;
	}

private:
	void throw_if_interface_not_set()
	{
		if(server == nullptr)
		{
			throw rpc::exception("interface not set");
		}
	}

	std::string run_command(const std::string &cmd)
	{
		try
		{
			nlohmann::json json = nlohmann::json::parse(cmd);
			nlohmann::json out;

			throw_if_interface_not_set();
			process_call(server, json, out);
			
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

	interface_server *server = nullptr;
};


typedef std::vector<uint8_t> buf_t;
typedef std::shared_ptr<buf_t> buf_ptr;

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
		
		virtual ~message(){}
		
		void reply(const buf_ptr &reply)
		{
			auto sp = master.lock();
			if(sp)
			{
				sp->reply(this, reply);
			}
			printf("%s: %d %s\n", __PRETTY_FUNCTION__, (int)reply->size(), sp ? "ok" : "false");
		}
		
		virtual buf_ptr get_buf() = 0;
		
		std::weak_ptr<channel_server> master;
	};	
	
	channel_server()
	{
		this_ref = this_ref_t(this, [](channel_server *){});
	}
	
	virtual ~channel_server() = default;
	
	virtual void stop(){}
	
	typedef std::shared_ptr<message> message_ptr;
	
	virtual message_ptr read_next() = 0;
	
	//virtual void reply(message_ptr m, const buf_t &reply) = 0;
	virtual void reply(message *m, const buf_ptr &reply) = 0;
	
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
	
	std::any call(const std::string method, const std::vector<std::any> &args) override
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
		
		auto req_str = call.dump();
		buf_t req_buf(req_str.begin(), req_str.end());
		
		if(client)
		{
			buf_t rep;
			client->send(req_buf, rep);
			return rep;
		}
		
		return std::any();
	}

	void set_client(channel_client *cl)
	{
		set_client(std::shared_ptr<channel_client>(cl, [](channel_client *){}));
	}
	
	void set_client(std::shared_ptr<channel_client> cl)
	{
		client = cl;
	}
	
private:	
	serializers_t serializers;
	deserializers_t deserializers;
	type_map_t type_map_in;
	type_map_t type_map_out;
	
	std::shared_ptr<channel_client> client;
};



class channel_inprocess : public channel_client, public channel_server
{
public:
	channel_inprocess()
	{
		//
	}
	
	~channel_inprocess()
	{
		running = false;
		queue_condvar.notify_all();
	}
	
	void send(const buf_t &request, buf_t &reply) override
	{
		std::unique_lock<std::mutex> lock(waiters_mutex);
		auto wtr = std::make_shared<waiter>();
		auto id = std::this_thread::get_id();
		waiters_map[id] = wtr;
		std::unique_lock<std::mutex> queue_lock(queue_mtx);
		buf_queue.push(id);
		queue_lock.unlock();
		queue_condvar.notify_one();
		lock.unlock();
		buf_ptr buf = wtr->wait(std::make_shared<buf_t>(request));
		reply = *(buf.get());
	}
	
	void send(const buf_t &buf) override
	{
		//
	}
	
	void stop()
	{
		running = false;
		queue_condvar.notify_all();
	}
	
	message_ptr read_next() override
	{
		std::unique_lock<std::mutex> queue_lock(queue_mtx);
		queue_condvar.wait(queue_lock, [this](){return (buf_queue.size() > 0) || (running == false);});
		if(running == false)
		{
			return message_ptr();
		}
		auto b = buf_queue.front();
		buf_queue.pop();
		queue_lock.unlock();
		std::unique_lock<std::mutex> waiters_lock(waiters_mutex);
		auto m = std::make_shared<inproc_message>(this_ref, b);
		m->buf = waiters_map.at(b)->req_buf;
		return m;
	}
	
private:
	typedef std::queue<std::thread::id> buf_queue_t;
	buf_queue_t buf_queue;
	std::mutex queue_mtx;
	std::condition_variable queue_condvar;
	bool running = true;
	
	
	class inproc_message : public message
	{
	public:
		inproc_message(channel_server::this_ref_t ref, std::thread::id i) : message(ref), id(i)
		{
			//
		}
		
		buf_ptr get_buf() override
		{
			return buf;
		}
		
		std::thread::id id;
		buf_ptr buf;
	};
	
	
	class waiter
	{
	public:		
		buf_ptr wait(buf_ptr req)
		{
			std::unique_lock<std::mutex> lock(mtx);
			req_buf = req;
			cv.wait(lock, [this](){return rep_buf.get() != nullptr;});
			return rep_buf;
		}
		
		void reply(const buf_ptr b)
		{
			std::unique_lock<std::mutex> lock(mtx);
			rep_buf = b;
			cv.notify_one();
		}
	private:
		std::mutex mtx;
		std::condition_variable cv;
	public:
		std::shared_ptr<buf_t> req_buf;
		std::shared_ptr<buf_t> rep_buf;
	};
	
	typedef std::shared_ptr<waiter> waiter_ptr;
	typedef std::map<std::thread::id, waiter_ptr> waiter_map_t;
	waiter_map_t waiters_map;
	std::mutex waiters_mutex;
	
	void reply(message *m, const buf_ptr &reply) override
	{
		inproc_message *im = dynamic_cast<inproc_message *>(m);
		if(im == nullptr)
		{
			return;
		}
		auto id = im->id;
		
		std::unique_lock<std::mutex> lock(queue_mtx);
		auto it = waiters_map.find(id);
		if(it == waiters_map.end())
		{
			return;
		}
		auto w = it->second;
		w->reply(reply);
		waiters_map.erase(it);
	}
};


class cmd : public command
{
public:
	cmd(channel_server::message_ptr m) : msg(m)
	{
		//
	}
	
	std::string get_cmd() const override
	{
		return msg.get() ? std::string((char *)msg->get_buf()->data(), msg->get_buf()->size()) : "";
	}
	
	void reply(const std::string &result) override
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, nlohmann::json::parse(result).dump(1, '\t').c_str());
		if(msg)
		{
			buf_t buf((uint8_t *)result.data(), (uint8_t *)result.data() + result.size());
			msg->reply(std::make_shared<buf_t>(buf));
		}
	}
	
	void error(const std::string &err) override
	{
		printf("%s: %s\n", __PRETTY_FUNCTION__, err.c_str());
		if(msg)
		{
			buf_t buf((uint8_t *)err.data(), (uint8_t *)err.data() + err.size());
			msg->reply(std::make_shared<buf_t>(buf));
		}
	}

private:
	channel_server::message_ptr msg;
};


class channel_poller
{
public:
	channel_poller()
	{
		is_running = true;
		thread = std::thread(&channel_poller::loop, this);
	}
	
	virtual ~channel_poller()
	{
		if(server)
		{
			server->stop();
		}
		is_running = false;
		if(thread.joinable())
		{
			thread.join();
		}
	}
	
	void set_executor(interface_executor *e)
	{
		iex = e;
	}
	
	void set_channel(channel_server *s)
	{
		server = s;
	}
	
private:
	interface_executor *iex = nullptr;
	channel_server *server = nullptr;
	
	std::thread thread;
	bool is_running = false;
	
	void loop()
	{
		for( ; is_running == true ; )
		{
			if(server == nullptr || iex == nullptr)
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				continue;
			}
			
			auto m = server->read_next();
			if(!m)
			{
				continue;
			}
			iex->add_command(std::make_shared<cmd>(m));
		}
	}
};



int main()
{
	channel_inprocess channel;
	
	testo tes;

	interface_executor executor;
	executor.set_interface(&tes);
	
	channel_poller poller;
	poller.set_executor(&executor);
	poller.set_channel(&channel);

	interface_client client;
	json_forwarder forwarder;
	client.fwd = &forwarder;	
	forwarder.set_client(&channel);
	
	client.call<int, std::string, int>("test", "kek", 17);
	
	std::this_thread::sleep_for(std::chrono::seconds(1));
	
	return 0;
}
