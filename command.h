#pragma once

#include <string>

namespace rpc
{

class command
{
public:
	command() = default;
	virtual ~command() = default;

	virtual std::string get_cmd() const = 0;
	virtual void reply(const std::string &cmd) = 0;
	virtual void error(const std::string &err) = 0;
};

}
