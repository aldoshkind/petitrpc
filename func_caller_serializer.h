#pragma once

#include <cereal/archives/json.hpp>

#include "func_caller.h"
#include "json_serialization.h"

namespace rpc::json_serialization
{

/*template <class R, class ... Ts>
void serialize(const func_caller<R, Ts...> fc, cereal::JSONOutputArchive &a, type_map_t &types)*/

void serialize(const func_caller_base *fc, cereal::JSONOutputArchive &a, const type_map_t &types)
{
	a(cereal::make_nvp("name", fc->name));
	a.setNextName("parameters");
	a.startNode();
	for(const auto &p : fc->params)
	{
		a(cereal::make_nvp(p.second, get_type(p.first, types)));
	}
	a.finishNode();
}

}
