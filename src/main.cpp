#include <iostream>

#include "engine/Core.hpp"


int main(int argc, char** argv) {
	Engine::Core engineCore;

	auto result = engineCore.init(argc, argv);
	if (result) {
		// TODO: display error message box
		return result;
	}

	auto exitCode = engineCore.run();
	if (exitCode) {
		// TODO: display error message box
		return exitCode;
	}

	return 0;
}
