#include "stdafx.h"
#include "INIFile.h"

#include <utility>
#include <cctype>
#include <algorithm>
#include <locale>

inline void trim_left(std::string& str)
{
	str.erase(str.begin(), std::ranges::find_if(str, [](int ch)
	{
		return !std::isspace(ch);
	}));
}

inline void trim_right(std::string& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch)
	{
		return !std::isspace(ch);
	}).base(), str.end());
}

inline void trim(std::string& str)
{
	trim_left(str);
	trim_right(str);
}

bool INISection::contains(const std::string& key) const
{
	return pairs.contains(key);
}

#pragma region parsing

template <>
float INISection::parse_value<float>(const std::string& value) const
{
	return std::stof(value);
}

template <>
double INISection::parse_value<double>(const std::string& value) const
{
	return std::stod(value);
}

template <>
int16_t INISection::parse_value<int16_t>(const std::string& value) const
{
	return static_cast<int16_t>(std::stol(value));
}

template <>
uint16_t INISection::parse_value<uint16_t>(const std::string& value) const
{
	return static_cast<int16_t>(std::stoul(value));
}

template <>
int32_t INISection::parse_value<int32_t>(const std::string& value) const
{
	return static_cast<int32_t>(std::stol(value));
}

template <>
uint32_t INISection::parse_value<uint32_t>(const std::string& value) const
{
	return static_cast<uint32_t>(std::stoul(value));
}

template <>
int64_t INISection::parse_value<int64_t>(const std::string& value) const
{
	return std::stoll(value);
}

template <>
uint64_t INISection::parse_value<uint64_t>(const std::string& value) const
{
	return std::stoull(value);
}

template <>
bool INISection::parse_value<bool>(const std::string& value) const
{
	if (value == "true" || value == "1")
	{
		return true;
	}

	if (value == "false" || value == "0")
	{
		return false;
	}

	throw std::exception("boolean values must either be true/false or 1/0, but the given value was not");
}

#pragma endregion

template <>
std::string INISection::get<std::string>(const std::string& key) const
{
	const std::string& value = pairs.at(key);
	return value;
}

template <>
[[nodiscard]] std::optional<std::string> INISection::try_get<std::string>(const std::string& key) const
{
	const auto it = pairs.find(key);

	if (it == pairs.end())
	{
		return std::nullopt;
	}

	return it->second;
}

template <>
void INISection::set<float>(const std::string& key, const float& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<double>(const std::string& key, const double& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<int16_t>(const std::string& key, const int16_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<uint16_t>(const std::string& key, const uint16_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<int32_t>(const std::string& key, const int32_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<uint32_t>(const std::string& key, const uint32_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<int64_t>(const std::string& key, const int64_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<uint64_t>(const std::string& key, const uint64_t& value)
{
	pairs[key] = std::to_string(value);
}

template <>
void INISection::set<bool>(const std::string& key, const bool& value)
{
	pairs[key] = value ? "true" : "false";
}

template <>
void INISection::set<std::string>(const std::string& key, const std::string& value)
{
	pairs[key] = value;
}

bool INISection::erase(const std::string& key)
{
	const auto it = pairs.find(key);

	if (it == pairs.end())
	{
		return false;
	}

	pairs.erase(it);
	return true;
}

void INIFile::read(std::fstream& stream)
{
	static const std::string assignment_delimiter = "=";
	std::string line;

	std::shared_ptr<INISection> section_ptr;

	while (std::getline(stream, line))
	{
		trim(line);

		if (line.empty())
		{
			continue;
		}

		// skip comment
		if (line.starts_with(';'))
		{
			continue;
		}

		if (line.starts_with('[') && line.ends_with(']'))
		{
			const std::string section_name = std::string(line.begin() + 1, line.end() - 1);
			section_ptr = std::make_shared<INISection>();

			set_section(section_name, section_ptr);
			continue;
		}

		if (!section_ptr)
		{
			continue;
		}

		const auto assignment = line.find(assignment_delimiter);

		if (assignment == std::string::npos)
		{
			continue;
		}

		std::string key = line.substr(0, assignment);
		trim(key);

		std::string value = line.substr(assignment + assignment_delimiter.length());
		trim(value);

		section_ptr->set(key, value);
	}
}

void INIFile::write(std::fstream& stream)
{
	auto write_section = [&](const decltype(sections)::value_type& section_pair)
	{
		stream << "[" << section_pair.first << "]" << std::endl;

		for (auto& [key, value] : section_pair.second->pairs)
		{
			stream << key << " = " << value << std::endl;
		}
	};

	// we do this to separate each [section] by a blank line efficiently

	auto it = sections.begin();

	if (it == sections.end())
	{
		return;
	}

	write_section(*it);
	++it;

	while (it != sections.end())
	{
		stream << std::endl;
		write_section(*it);
		++it;
	}
}

bool INIFile::contains_section(const std::string& section_name) const
{
	return sections.contains(section_name);
}

std::shared_ptr<INISection> INIFile::get_section(const std::string& section_name) const
{
	const auto it = sections.find(section_name);

	if (it == sections.cend())
	{
		return nullptr;
	}

	return it->second;
}

void INIFile::set_section(const std::string& section_name, std::shared_ptr<INISection> section_ptr)
{
	if (!section_ptr)
	{
		sections.erase(section_name);
	}
	else
	{
		sections[section_name] = std::move(section_ptr);
	}
}
