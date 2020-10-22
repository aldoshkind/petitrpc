#pragma once

#include <any>
#include <list>
#include <string>
#include <vector>

namespace rpc
{

typedef std::vector<std::any> arg_set_t;
typedef std::list<std::pair<std::string, std::string>> param_descr_list_t;

class func_caller_base
{
public:
	virtual ~func_caller_base() = default;

	std::string name;
	std::string return_type;
	param_descr_list_t params;

	void set_param_name(size_t i, const std::string &name)
	{
		if(params.size() > i)
		{
			std::next(params.begin(), i)->second = name;
		}
	}

	virtual std::any call(const arg_set_t &a) = 0;
};

}
