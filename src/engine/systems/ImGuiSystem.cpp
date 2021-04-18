#include "ImGuiSystem.hpp"

#include "thirdparty/imgui/imgui_impl_glfw.h"

#include <spdlog/spdlog.h>


namespace Engine::Systems {
int ImGuiSystem::init() {
	spdlog::info("Initializing ImGuiSystem...");

	assert(glfwWindow != nullptr);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

	return 0;
}


int ImGuiSystem::run(double dt) {
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

	return 0;
}
} // namespace Engine::Systems