#pragma once

#include <nlohmann/json.hpp>

#include "func_caller.h"
#include "json_serialization.h"

namespace rpc::json_serialization
{

/*template <class R, class ... Ts>
void serialize(const func_caller<R, Ts...> fc, cereal::JSONOutputArchive &a, type_map_t &types)*/

void serialize(const func_caller_base *fc, nlohmann::json &a, const type_map_t &types)
{
	a["name"] = fc->name;
	nlohmann::json params = a.array({});
	for(const auto &p : fc->params)
	{
		params[p.second] = get_type(p.first, types);
	}
}

}
