#include "json_serialization.h"

namespace rpc::json_serialization
{

std::string get_type(const std::string &id, const type_map_t &types)
{
	if(types.find(id) == types.end())
	{
		return id;
	}
	return types.at(id);
}

bool deserialize(const nlohmann::json &arch, const deserializers_t &desers, std::any &a, std::string type)
{
	auto deser = desers.find(type);
	if(deser == desers.end())
	{
		return false;
	}
	deser->second(arch, a);
	return true;
}

void init_types(serializers_t &serializers, deserializers_t &deserializers, type_map_t &type_map_in, type_map_t &type_map_out)
{
	add_type<int>(serializers, deserializers);
	add_type<float>(serializers, deserializers);
	add_type<double>(serializers, deserializers);
    add_type<bool>(serializers, deserializers);
	add_type<uint32_t>(serializers, deserializers);
	add_type<uint16_t>(serializers, deserializers);
	add_type<int32_t>(serializers, deserializers);
	add_type<int16_t>(serializers, deserializers);
	add_type<int8_t>(serializers, deserializers);
	add_type<uint8_t>(serializers, deserializers);
	add_type<std::string>(serializers, deserializers);

	add_type<uint8_t>("uint8", type_map_in, type_map_out);
	add_type<uint16_t>("uint16", type_map_in, type_map_out);
	add_type<uint32_t>("uint32", type_map_in, type_map_out);
	add_type<uint64_t>("uint64", type_map_in, type_map_out);
	add_type<int8_t>("int8", type_map_in, type_map_out);
	add_type<int16_t>("int16", type_map_in, type_map_out);
	add_type<int32_t>("int32", type_map_in, type_map_out);
	add_type<int64_t>("int64", type_map_in, type_map_out);
	add_type<float>("float", type_map_in, type_map_out);
	add_type<double>("double", type_map_in, type_map_out);
	add_type<std::string>("string", type_map_in, type_map_out);
	add_type<void>("void", type_map_in, type_map_out);
    add_type<bool>("bool", type_map_in, type_map_out);

	serializers[typeid(void).name()] = [](const std::any &, nlohmann::json &j, const serializer *){j.clear();};
	deserializers[typeid(void).name()] = [](const nlohmann::json &, std::any &a){a.reset();};
}

}
