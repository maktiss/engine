#include "ImGuiSystem.hpp"

#include "thirdparty/imgui/imgui_impl_glfw.h"

#include <spdlog/spdlog.h>


namespace Engine {
int ImGuiSystem::init() {
	spdlog::info("Initializing ImGuiSystem...");

	assert(glfwWindow != nullptr);


	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

	return 0;
}


int ImGuiSystem::run(double dt) {
	auto& imGuiState = GlobalStateManager::getWritable<ImGuiState>();

	if (InputManager::isKeyActionStatePressedOnce(InputManager::KeyAction::IMGUI_TOGGLE)) {
		imGuiState.showUI = !imGuiState.showUI;
	}

	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (imGuiState.showUI) {
		auto mainWindowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
							   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
							   ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
							   ImGuiWindowFlags_NoBackground;

		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });

		ImGui::Begin("Main Window", nullptr, mainWindowFlags);

		ImGui::PopStyleVar(3);

		ImGui::DockSpace(ImGui::GetID("DockSpace"), {}, ImGuiDockNodeFlags_PassthruCentralNode);

		showLogWindow();
		showStatisticsWindow();

		ImGui::ShowDemoWindow();

		ImGui::End();
	}

	ImGui::Render();

	return 0;
}


void ImGuiSystem::showLogWindow() {
	auto& debugState = GlobalStateManager::get<DebugState>();

	if (ImGui::Begin("Log")) {
		for (uint i = debugState.logOffset; i < debugState.logCount; i++) {
			ImGui::Text(debugState.logs[i].data());
		}
		for (uint i = 0; i < debugState.logOffset; i++) {
			ImGui::Text(debugState.logs[i].data());
		}
	}
	ImGui::End();
}

void ImGuiSystem::showStatisticsWindow() {
	auto& debugState = GlobalStateManager::get<DebugState>();

	if (ImGui::Begin("Statistics", nullptr)) {
		ImGui::Text("Avg. Frame Time: %.3f ms (%.1f fps)", debugState.avgFrameTime * 1000.0f,
					1.0f / debugState.avgFrameTime);

		if (ImGui::BeginTable("Table", 3, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("System");
			ImGui::TableSetupColumn("CPU Time");
			ImGui::TableSetupColumn("GPU Time");
			// ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			for (uint column = 0; column < 3; column++) {
				ImGui::TableNextColumn();
				if (column == 0) {
					ImGui::Indent();
				}
				ImGui::TableHeader(ImGui::TableGetColumnName(column));
				if (column == 0) {
					ImGui::Unindent();
				}
			}

			int currentTreeLevel = 0;
			for (const auto& executionTimes : debugState.avgExecutionTimeArrays) {
				for (uint i = 0; i < executionTimes.size(); i++) {
					const auto& executionTime = executionTimes[i];

					if (currentTreeLevel == executionTime.level) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();

						bool buildTree = false;
						if (i < executionTimes.size() - 1) {
							buildTree = executionTime.level < executionTimes[i + 1].level;
						}

						if (buildTree) {
							ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 8.0f);
							if (ImGui::TreeNode(executionTime.name.c_str())) {
								currentTreeLevel++;
							}
							ImGui::PopStyleVar();
						} else {
							ImGui::Indent();
							ImGui::Text(executionTime.name.c_str());
							ImGui::Unindent();
						}

						const auto indentSize = std::max(8.0f * executionTime.level, 0.0001f);

						ImGui::TableNextColumn();
						ImGui::Indent(indentSize);
						if (executionTime.cpuTime >= 0.0f) {
							ImGui::Text("%.3f ms", executionTime.cpuTime * 1000.0f);
						} else {
							ImGui::Text("--");
						}
						ImGui::Unindent(indentSize);

						ImGui::TableNextColumn();
						ImGui::Indent(indentSize);
						if (executionTime.gpuTime >= 0.0f) {
							ImGui::Text("%.3f ms", executionTime.gpuTime);
						} else {
							ImGui::Text("--");
						}
						ImGui::Unindent(indentSize);
					}

					int targetLevel = 0;
					if (i < executionTimes.size() - 1) {
						targetLevel = executionTimes[i + 1].level;
					}

					ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 8.0f);
					for (currentTreeLevel; currentTreeLevel > targetLevel; currentTreeLevel--) {
						ImGui::TreePop();
					}
					ImGui::PopStyleVar();
				}
			}
			assert(currentTreeLevel == 0);

			ImGui::EndTable();
		}
	}
	ImGui::End();
}
} // namespace Engine