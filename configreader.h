#pragma once

class ConfigReader
{
public:
	ConfigReader() = default;
	~ConfigReader() = default;
	bool ReadConfig();
	static std::string GetCurrentDir();
	nlohmann::json config_;
};