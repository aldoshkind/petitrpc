#pragma once

#include <string>
#include <exception>

namespace rpc
{

class exception : public std::exception
{
public:
	exception(const std::string &message) : msg(message)
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

}
