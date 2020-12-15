#include "func_caller_serializer.h"

namespace rpc::json_serialization
{

void serialize_caller(const func_caller_base *fc, nlohmann::json &a, const type_map_t &types)
{
	a["name"] = fc->name;
	a["return"] = get_type(fc->return_type, types);
	auto &params = a["params"];
	for(const auto &p : fc->params)
	{
		params[p.second] = get_type(p.first, types);
	}
}

}
