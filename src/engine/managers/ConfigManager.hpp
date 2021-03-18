#pragma once

#include <spdlog/spdlog.h>

#include <map>
#include <sstream>
#include <string>


#define PROPERTY(type, category, name, defaultValue)       \
	Engine::Managers::ConfigManager::Property<type> name = \
		Engine::Managers::ConfigManager::Property<type>(category "." #name, defaultValue)


namespace Engine::Managers {
class ConfigManager {
public:
	template <typename PropertyType>
	class Property {
	private:
		PropertyType value {};

	public:
		Property(std::string key, PropertyType defaultValue) {
			value = ConfigManager::getProperty(key, defaultValue);
		}

		inline operator PropertyType() const {
			return value;
		}
	};


private:
	static std::map<std::string, std::string> propertyMap;

	static bool configLoaded;


public:
	static int setProperty(std::string key, std::string value) {
		if (!isKeyValid(key)) {
			spdlog::error("[ConfigManager] Key '{}' is invalid", key);
			return 1;
		}

		propertyMap[key] = value;

		// FIXME: save on exit
		saveConfig();
		return 0;
	}

	template <typename PropertyType>
	static PropertyType getProperty(std::string key, PropertyType defaultValue) {
		PropertyType value = defaultValue;

		if (!configLoaded) {
			if (loadConfig()) {
				return value;
			}
		}

		auto iter = propertyMap.find(key);
		if (iter == propertyMap.end()) {
			setProperty(key, toString(value));
		} else if (fromString(iter->second, value)) {
			spdlog::error("[ConfigManager] Failed to convert property '{}' from value '{}'", key, iter->second);

			value = defaultValue;
			setProperty(key, toString(value));
		}

		return value;
	}


	[[nodiscard]] static int loadConfig();
	[[nodiscard]] static int saveConfig();


private:
	ConfigManager() {
	}

	inline static bool isKeyValid(std::string key) {
		return key.find_first_of("=") == -1 && key.find('.') != -1;
	}

	template <typename PropertyType>
	[[nodiscard]] static int fromString(std::string stringValue, PropertyType& value) {
		std::stringstream buffer(stringValue);

		buffer >> value;
		if (!buffer.eof()) {
			return 1;
		}

		return 0;
	}

	template <typename PropertyType>
	static std::string toString(PropertyType value) {
		return std::to_string(value);
	}
};
} // namespace Engine::Managers