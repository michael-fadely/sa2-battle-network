#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>

// TODO: use std::hash style thing for parse functions

class INISection
{
public:
	std::map<std::string, std::string> pairs;

	[[nodiscard]] bool contains(const std::string& key) const;

	template <typename T>
	[[nodiscard]] T parse_value(const std::string& value) const = delete;

	template <typename T>
	[[nodiscard]] T get(const std::string& key) const
	{
		return parse_value<T>(pairs.at(key));
	}

	template <typename T>
	[[nodiscard]] std::optional<T> try_get(const std::string& key) const
	{
		const auto it = pairs.find(key);

		if (it == pairs.end())
		{
			return std::nullopt;
		}

		return parse_value<T>(it->second);
	}

	template <typename T>
	void set(const std::string& key, const T& value) = delete;

	bool erase(const std::string& key);
};

template <>
[[nodiscard]] float INISection::parse_value<float>(const std::string& value) const;

template <>
[[nodiscard]] double INISection::parse_value<double>(const std::string& value) const;

template <>
[[nodiscard]] int16_t INISection::parse_value<int16_t>(const std::string& value) const;

template <>
[[nodiscard]] uint16_t INISection::parse_value<uint16_t>(const std::string& value) const;

template <>
[[nodiscard]] int32_t INISection::parse_value<int32_t>(const std::string& value) const;

template <>
[[nodiscard]] uint32_t INISection::parse_value<uint32_t>(const std::string& value) const;

template <>
[[nodiscard]] int64_t INISection::parse_value<int64_t>(const std::string& value) const;

template <>
[[nodiscard]] uint64_t INISection::parse_value<uint64_t>(const std::string& value) const;

template <>
[[nodiscard]] bool INISection::parse_value<bool>(const std::string& value) const;

template <>
[[nodiscard]] std::string INISection::get<std::string>(const std::string& key) const;

template <>
[[nodiscard]] std::optional<std::string> INISection::try_get<std::string>(const std::string& key) const;

template <>
void INISection::set<float>(const std::string& key, const float& value);

template <>
void INISection::set<double>(const std::string& key, const double& value);

template <>
void INISection::set<int16_t>(const std::string& key, const int16_t& value);

template <>
void INISection::set<uint16_t>(const std::string& key, const uint16_t& value);

template <>
void INISection::set<int32_t>(const std::string& key, const int32_t& value);

template <>
void INISection::set<uint32_t>(const std::string& key, const uint32_t& value);

template <>
void INISection::set<int64_t>(const std::string& key, const int64_t& value);

template <>
void INISection::set<uint64_t>(const std::string& key, const uint64_t& value);

template <>
void INISection::set<bool>(const std::string& key, const bool& value);

template <>
void INISection::set<std::string>(const std::string& key, const std::string& value);

class INIFile
{
public:
	std::map<std::string, std::shared_ptr<INISection>> sections;

	INIFile() = default;

	void read(std::fstream& stream);
	void write(std::fstream& stream);

	[[nodiscard]] bool contains_section(const std::string& section_name) const;

	[[nodiscard]] std::shared_ptr<INISection> get_section(const std::string& section_name) const;
	void set_section(const std::string& section_name, std::shared_ptr<INISection> section_ptr);
};
