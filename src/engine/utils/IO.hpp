#pragma once

#include <fstream>
#include <string>
#include <vector>


namespace Engine::Utils {
inline int readFile(std::string filename, std::vector<uint8_t>& buffer) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file) {
		return 1;
	}

	auto fileSize = file.tellg();
	buffer.resize(fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	return 0;
}
} // namespace Engine::Utils