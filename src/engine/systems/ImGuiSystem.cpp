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

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

	return 0;
}


int ImGuiSystem::run(double dt) {

	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();


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

	ImGui::Begin("Main Window", nullptr,
				 mainWindowFlags);

	ImGui::PopStyleVar(3);

	ImGui::DockSpace(ImGui::GetID("DockSpace"), {}, ImGuiDockNodeFlags_PassthruCentralNode);

	showLogWindow();
	showStatisticsWindow();

	ImGui::ShowDemoWindow();

	ImGui::End();

	ImGui::Render();

	return 0;
}


void ImGuiSystem::showLogWindow() {
	auto& debugState = Engine::Managers::GlobalStateManager::get<Engine::States::DebugState>();

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
	auto& debugState = Engine::Managers::GlobalStateManager::get<Engine::States::DebugState>();

	if (ImGui::Begin("Statistics", nullptr)) {
		ImGui::Text("Avg. Frame Time: %.3f ms (%.1f fps)", debugState.avgFrameTime * 1000.0f,
					1.0f / debugState.avgFrameTime);

		if (ImGui::BeginTable("Table", 2, ImGuiTableFlags_PadOuterX | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("Renderer");
			ImGui::TableSetupColumn("GPU Time");
			// ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			for (uint column = 0; column < 2; column++) {
				ImGui::TableNextColumn();
				if (column == 0) {
					ImGui::Indent();
				}
				ImGui::TableHeader(ImGui::TableGetColumnName(column));
				if (column == 0) {
					ImGui::Unindent();
				}
			}

			for (const auto& [rendererName, times] : debugState.rendererExecutionTimes) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				double totalTime = 0.0;
				for (const auto& time : times) {
					totalTime += time;
				}

				if (times.size() > 1) {
					if (ImGui::TreeNode(rendererName.c_str())) {
						ImGui::TableNextColumn();
						ImGui::Text("%.3f ms", totalTime);

						for (uint i = 0; i < times.size(); i++) {
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text(" Layer %d", i);

							ImGui::TableNextColumn();
							ImGui::Text(" %.3f ms", times[i]);
						}

						ImGui::TreePop();
					} else {
						ImGui::TableNextColumn();
						ImGui::Text("%.3f ms", totalTime);
					}
				} else {
					ImGui::Indent();
					ImGui::Text(rendererName.c_str());
					ImGui::Unindent();

					ImGui::TableNextColumn();
					ImGui::Text("%.3f ms", totalTime);
				}
			}

			ImGui::EndTable();
		}
	}
	ImGui::End();
}
} // namespace Engine::Systems