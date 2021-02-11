#include "func_caller_serializer.h"

namespace rpc::json_serialization
{

void serialize_caller(const func_caller_base *fc, nlohmann::json &a, const type_map_t &types)
{
    a["name"] = fc->name;
    a["return"] = get_type(fc->return_type, types);

    auto params = nlohmann::json::array();
    nlohmann::json parameter_json;
    for(const auto &p : fc->params)
    {
        parameter_json["name"] = p.second;
        parameter_json["type"] = get_type(p.first, types);
        params.push_back(parameter_json);
    }

    a["arguments"] = params;
}

}
