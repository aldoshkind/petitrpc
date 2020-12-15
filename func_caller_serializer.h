#pragma once

#include <nlohmann/json.hpp>

#include "func_caller.h"
#include "json_serialization.h"

namespace rpc::json_serialization
{

void serialize_caller(const func_caller_base *fc, nlohmann::json &a, const type_map_t &types);

}

