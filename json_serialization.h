#pragma once

#include <map>
#include <any>
#include <string>
#include <functional>

#include <nlohmann/json.hpp>

namespace rpc::json_serialization
{

typedef std::map<std::string, std::string> type_map_t;

class serializer
{
public:
	virtual ~serializer() {}
	virtual bool serialize(const std::any &, nlohmann::json &) const = 0;
};

typedef std::function<void(const std::any &, nlohmann::json &, const serializer *)> serializer_t;
typedef std::function<void(const nlohmann::json &, std::any &)> deserializer_t;

typedef std::map<std::string, serializer_t> serializers_t;
typedef std::map<std::string, deserializer_t> deserializers_t;



template <class T>
void serialize(const std::any &a, nlohmann::json &b, const serializer *)
{
	const auto &v = std::any_cast<T>(a);
	b = v;
}

template <class T>
void deserialize(const nlohmann::json &b, std::any &a)
{
	T v = b;
	a = v;
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

std::string get_type(const std::string &id, const type_map_t &types);
bool deserialize(const nlohmann::json &arch, const deserializers_t &desers, std::any &a, std::string type);
void init_types(serializers_t &serializers, deserializers_t &deserializers, type_map_t &type_map_in, type_map_t &type_map_out);

}
