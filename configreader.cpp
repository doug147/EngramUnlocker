#include "framework.h"

std::string ConfigReader::GetCurrentDir()
{
	char buffer[MAX_PATH];

	GetModuleFileNameA(nullptr, buffer, MAX_PATH);

	const std::string::size_type pos = std::string(buffer).find_last_of("\\/");
	return std::string(buffer).substr(0, pos);
}

bool ConfigReader::ReadConfig()
{
	const std::string config_path = GetCurrentDir() + "/ArkApi/Plugins/MyEngramUnlocker/config.json";
	std::ifstream file{ config_path };
	if (!file.is_open())
	{
		return false;
	}

	file >> config_;
	file.close();

	return true;
}