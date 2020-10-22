#pragma once

#include <map>
#include <any>
#include <string>
#include <functional>

#include <nlohmann/json.hpp>

namespace rpc::json_serialization
{

typedef std::map<std::string, std::string> type_map_t;

typedef std::function<void(const std::any &, nlohmann::json &)> serializer_t;
typedef std::function<void(nlohmann::json &, std::any &)> deserializer_t;

typedef std::map<std::string, serializer_t> serializers_t;
typedef std::map<std::string, deserializer_t> deserializers_t;

template <class T>
void serialize(const std::any &a, nlohmann::json &b)
{
	const auto &v = std::any_cast<T>(a);
	b = v;
}

template <class T>
void deserialize(nlohmann::json &b, std::any &a)
{
	a = b;
}

template <class T>
void add_serializer(serializers_t &s)
{
	s[typeid (T).name()] = serialize<T>;
}

template <class T>
void add_deserializer(deserializers_t &s)
{
	s[typeid (T).name()] = deserialize<T>;
}

template <class T>
void add_type(serializers_t &s, deserializers_t &d)
{
	add_serializer<T>(s);
	add_deserializer<T>(d);
}

template <class T>
void add_type(const std::string &name, type_map_t &in, type_map_t &out)
{
	std::string id = typeid(T).name();
	in[name] = id;
	out[id] = name;
}

std::string get_type(const std::string &id, const type_map_t &types)
{
	if(types.find(id) == types.end())
	{
		return id;
	}
	return types.at(id);
}

bool serialize(const std::any &a, const serializers_t &sers, nlohmann::json &arch)
{
	auto ser = sers.find(a.type().name());
	if(ser == sers.end())
	{
		return false;
	}
	ser->second(a, arch);
	return true;
}

bool deserialize(nlohmann::json &arch, const deserializers_t &desers, std::any &a, std::string type)
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
}

}
