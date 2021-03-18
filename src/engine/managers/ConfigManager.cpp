#include "ConfigManager.hpp"

#include <fstream>


namespace Engine::Managers {
std::map<std::string, std::string> ConfigManager::propertyMap {};
bool ConfigManager::configLoaded {};


template <>
std::string ConfigManager::toString(std::string value) {
	return value;
}


int ConfigManager::loadConfig() {
	const std::string fileName = "Engine.ini";
	std::ifstream configFile(fileName);

	std::string line;
	uint lineIndex = 0;

	std::string category = "INVALID";

	while (std::getline(configFile, line)) {
		lineIndex++;

		if (line.size() != 0) {
			if (line[0] == '[') {
				if (line.size() >= 3 && line[line.size() - 1] == ']') {
					category = line.substr(1, line.size() - 2);
				} else {
					spdlog::error("[ConfigManager] {} ({}): syntax error", fileName, lineIndex);
				}
			} else {
				auto splitPos = line.find('=');
				if (splitPos == 0 || splitPos == -1) {
					spdlog::error("[ConfigManager] {} ({}): syntax error", fileName, lineIndex);
				} else {
					auto key   = category + "." + line.substr(0, splitPos);
					auto value = line.substr(splitPos + 1);

					if (setProperty(key, value)) {
						spdlog::error("[ConfigManager] {} ({}): syntax error", fileName, lineIndex);
					}
				}
			}
		}
	}

	configLoaded = true;

	return 0;
}

int ConfigManager::saveConfig() {
	const std::string fileName = "Engine.ini";
	std::ofstream configFile(fileName);

	bool firstCategory = true;

	std::string category = "INVALID";
	for (const auto& [key, value] : propertyMap) {
		auto splitPos = key.find('.');

		auto currentCategory = key.substr(0, splitPos);
		auto actualKey = key.substr(splitPos + 1);

		if (category != currentCategory) {
			category = currentCategory;
			if (firstCategory) {
				firstCategory = false;
			} else {
				configFile << std::endl;
			}
			configFile << "[" << category << "]" << std::endl;
		}

		configFile << actualKey << "=" << value << std::endl;
	}

	return 0;
}
} // namespace Engine::Managers