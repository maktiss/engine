#pragma once

#include "MaterialManager.hpp"
#include "MeshManager.hpp"

#include "engine/utils/IO.hpp"

#include <spdlog/spdlog.h>

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include <shaderc/shaderc.hpp>

#include <array>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>


namespace Engine::Managers {
template <typename DerivedManager, typename... ShaderTypes>
class ShaderManagerBase {
public:
	class Handle {
		uint32_t index = 0;

	public:
		Handle() {};

		Handle(uint32_t index) : index(index) {};

		inline uint32_t getIndex() {
			return index;
		}
	};

	struct ShaderInfo {
		// TODO: access with enum
		std::array<vk::ShaderModule, 6> shaderModules;
	};

private:
	static std::tuple<std::vector<ShaderTypes>...> shaderObjectArrays;

	// Shader info arrays splitted by render pass
	static std::vector<std::vector<ShaderInfo>> shaderInfoArrays;

	// Shader type offsets within shaderInfos array
	static std::array<uint32_t, sizeof...(ShaderTypes)> shaderTypeOffsets;

	static vk::Device vkDevice;

public:
	static int init() {
		assert(vkDevice != vk::Device());

		// resize shader object arrays to fit all of their variations
		(std::get<std::vector<ShaderTypes>>(shaderObjectArrays)
			 .resize(pow(2, ShaderTypes::getFlagCount()) * MeshManager::getTypeCount() *
					 ShaderManagerBase::getRenderPassStringCount()),
		 ...);

		uint32_t totalShaderCount = 0;
		((totalShaderCount += shaderTypeOffsets[getTypeIndex<ShaderTypes>()] =
			  pow(2, ShaderTypes::getFlagCount()) * MeshManager::getTypeCount()),
		 ...);

		shaderInfoArrays.resize(ShaderManagerBase::getRenderPassStringCount());
		for (auto& shaderInfos : shaderInfoArrays) {
			shaderInfos.resize(totalShaderCount);
		}

		// calculate shader type mapping offsets
		for (uint i = shaderTypeOffsets.size() - 1; i > 0; i--) {
			uint32_t offset = 0;
			for (uint j = i - 1; j >= 0; j--) {
				offset += shaderTypeOffsets[j];
			}
			shaderTypeOffsets[i] = offset;
		}
		shaderTypeOffsets[0] = 0;

		return 0;
	}

	static Handle getHandle(MeshManager::Handle meshHandle, MaterialManager::Handle materialHandle) {
		auto shaderTypeIndex = getCompatibleShaderTypeIndex(materialHandle);

		auto meshTypeIndex = MeshManager::getTypeIndex(meshHandle);

		uint32_t materialFlags = 0;
		MaterialManager::apply(materialHandle, [&materialFlags](auto& material) {
			materialFlags = material.getFlags();
		});

		uint32_t index =
			shaderTypeOffsets[shaderTypeIndex] + meshTypeIndex * MeshManager::getTypeCount() + materialFlags;

		assert(index < shaderInfoArrays[0].size());
		return Handle(index);
	}

	static inline ShaderInfo& getShaderInfo(Handle handle, uint32_t renderPass) {
		assert(renderPass < ShaderManagerBase::getRenderPassStringCount());
		return shaderInfoArrays[renderPass][handle.getIndex()];
	}


	// Imports shader sources generating all variations
	template <typename ShaderType>
	static inline int importShaderSources(std::array<std::string, 6> glslSourceFilenames) {
		std::string logMessage = "Importing shader(s): ";
		bool commaNeeded	   = false;
		for (uint i = 0; i < 6; i++) {
			if (!glslSourceFilenames[i].empty()) {
				if (commaNeeded) {
					logMessage += ", ";
				}
				logMessage += "'";
				logMessage += glslSourceFilenames[i];
				logMessage += "'";

				commaNeeded = true;
			}
		}
		spdlog::info(logMessage + "...");

		constexpr auto shaderTypeIndex = getTypeIndex<ShaderType>();

		shaderc::Compiler compiler;

		std::array<std::string, 6> glslSources;

		for (uint i = 0; i < 6; i++) {
			if (!glslSourceFilenames[i].empty()) {
				if (Engine::Utils::readTextFile(glslSourceFilenames[i], glslSources[i])) {
					spdlog::error("Failed to open shader '{}' for compilation", glslSourceFilenames[i]);
					return 1;
				}

				std::string directoryName =
					glslSourceFilenames[i].substr(0, glslSourceFilenames[i].find_last_of("\\/"));

				if (!directoryName.empty()) {
					directoryName += "/";
				}

				// preprocess #include directives
				int position = 0;
				while ((position = glslSources[i].find("#include", position)) != -1) {
					auto includeLineLength = glslSources[i].find("\n", position) - position;

					auto filenamePosition = glslSources[i].find("\"", position);
					auto filenameLength	  = glslSources[i].find("\"", filenamePosition + 1) - filenamePosition - 1;

					if (position + includeLineLength < filenamePosition) {
						spdlog::error("Failed preprocess shader '{}': include errors detected",
									  glslSourceFilenames[i]);
						return 1;
					}

					std::string filename = directoryName + glslSources[i].substr(filenamePosition + 1, filenameLength);

					std::string includeSource;
					if (Engine::Utils::readTextFile(filename, includeSource)) {
						spdlog::error("Failed preprocess shader '{}': cannot open '{}' to include",
									  glslSourceFilenames[i],
									  filename);
						return 1;
					}

					glslSources[i].replace(position, includeLineLength, includeSource);
				}
			}
		}

		for (uint renderPassIndex = 0; renderPassIndex < getRenderPassStringCount(); renderPassIndex++) {
			for (uint meshTypeIndex = 0; meshTypeIndex < MeshManager::getTypeCount(); meshTypeIndex++) {
				for (uint flags = 0; flags < pow(2, ShaderType::getFlagCount()); flags++) {
					shaderc::CompileOptions options;

					options.SetOptimizationLevel(shaderc_optimization_level_size);

					options.AddMacroDefinition(DerivedManager::getRenderPassStrings()[renderPassIndex]);
					options.AddMacroDefinition(MeshManager::getMeshTypeString(meshTypeIndex));

					for (uint flagIndex = 0; flagIndex < ShaderType::getFlagCount(); flagIndex++) {
						if ((1 << flagIndex) & flags) {
							options.AddMacroDefinition(ShaderType::getFlagNames()[flagIndex]);
						}
					}

					uint32_t index = renderPassIndex * MeshManager::getTypeCount() * ShaderType::getFlagCount() +
									 meshTypeIndex * ShaderType::getFlagCount() + flags;

					for (uint shaderStageIndex = 0; shaderStageIndex < 6; shaderStageIndex++) {
						if (!glslSources[shaderStageIndex].empty()) {
							shaderc_shader_kind kind;
							switch (shaderStageIndex) {
							case 0:
								kind = shaderc_glsl_vertex_shader;
								break;
							case 1:
								kind = shaderc_glsl_geometry_shader;
								break;
							case 2:
								kind = shaderc_glsl_tess_control_shader;
								break;
							case 3:
								kind = shaderc_glsl_tess_evaluation_shader;
								break;
							case 4:
								kind = shaderc_glsl_fragment_shader;
								break;
							case 5:
								kind = shaderc_glsl_compute_shader;
								break;
							}

							shaderc::SpvCompilationResult result =
								compiler.CompileGlslToSpv(glslSources[shaderStageIndex],
														  kind,
														  glslSourceFilenames[shaderStageIndex].c_str(),
														  options);

							if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
								spdlog::error("Failed to compile shader '{}'. Error message:\n{}",
											  glslSourceFilenames[shaderStageIndex],
											  result.GetErrorMessage());

								return 1;
							}

							std::vector<uint32_t> spirvSource(result.cbegin(), result.cend());

							std::get<shaderTypeIndex>(shaderObjectArrays)[index].setShaderSource(
								shaderStageIndex, spirvSource.data(), spirvSource.size() * 4);
						}
					}
				}
			}
		}

		createShaderModules<ShaderType>();

		return 0;
	}

	template <typename ShaderType>
	static int createShaderModules() {
		assert(vkDevice != vk::Device());

		constexpr auto shaderTypeIndex = getTypeIndex<ShaderType>();

		for (uint32_t renderPassIndex = 0; renderPassIndex < getRenderPassStringCount(); renderPassIndex++) {
			for (uint32_t meshTypeIndex = 0; meshTypeIndex < MeshManager::getTypeCount(); meshTypeIndex++) {
				for (uint32_t signature = 0; signature < pow(2, ShaderType::getFlagCount()); signature++) {

					uint32_t index = getShaderIndex<ShaderType>(renderPassIndex, meshTypeIndex, signature);
					uint32_t shaderInfoIndex =
						getShaderInfoIndex<ShaderType>(shaderTypeIndex, meshTypeIndex, signature);

					for (uint shaderStageIndex = 0; shaderStageIndex < 6; shaderStageIndex++) {
						auto shaderSource =
							std::get<shaderTypeIndex>(shaderObjectArrays)[index].getShaderSource(shaderStageIndex);

						if (!shaderSource.empty()) {
							vk::ShaderModuleCreateInfo shaderModuleCreateInfo {};
							shaderModuleCreateInfo.codeSize = shaderSource.size();
							shaderModuleCreateInfo.pCode	= reinterpret_cast<const uint32_t*>(shaderSource.data());

							auto result = vkDevice.createShaderModule(
								&shaderModuleCreateInfo,
								nullptr,
								&shaderInfoArrays[renderPassIndex][shaderInfoIndex].shaderModules[shaderStageIndex]);

							if (result != vk::Result::eSuccess) {
								spdlog::error("[vulkan] Failed to create shader module. Error code: {} ({})",
											  result,
											  vk::to_string(result));
								return 1;
							}
						}
					}
				}
			}
		}

		return 0;
	}


	static constexpr uint32_t getRenderPassStringCount() {
		return DerivedManager::getRenderPassStrings().size();
	}

	// Returns render pass index given it's string
	static constexpr uint32_t getRenderPassIndex(const char* renderPassString) {
		constexpr auto& renderPassStrings = DerivedManager::getRenderPassStrings();
		uint32_t index					  = -1;
		for (uint i = 0; i < renderPassStrings.size(); i++) {
			if (strcmp(renderPassString, renderPassStrings[i]) == 0) {
				index = i;
				break;
			}
		}

		assert(index != -1);
		return index;
	}


	template <typename Type>
	static constexpr uint32_t getTypeIndex() {
		return getTypeIndexImpl<Type>(std::make_index_sequence<sizeof...(ShaderTypes)>());
	}

	template <typename Type, std::size_t... Indices>
	static constexpr uint32_t getTypeIndexImpl(std::index_sequence<Indices...>) {
		return ((std::is_same<typename std::tuple_element<Indices, std::tuple<ShaderTypes...>>::type, Type>::value
					 ? Indices
					 : 0) +
				...);
	}


	static void setVkDevice(vk::Device device) {
		vkDevice = device;
	}


protected:
	ShaderManagerBase() {
	}


private:
	template <typename ShaderType>
	static inline uint32_t getShaderIndex(uint32_t renderPassIndex, uint32_t meshTypeIndex, uint32_t signature) {
		return renderPassIndex * MeshManager::getTypeCount() * ShaderType::getFlagCount() +
			   meshTypeIndex * ShaderType::getFlagCount() + signature;
	}

	template <typename ShaderType>
	static inline uint32_t getShaderInfoIndex(uint32_t shaderTypeIndex, uint32_t meshTypeIndex, uint32_t signature) {
		return shaderTypeOffsets[shaderTypeIndex] + meshTypeIndex * ShaderType::getFlagCount() + signature;
	}


	static uint32_t getCompatibleShaderTypeIndex(MaterialManager::Handle materialHandle) {
		auto materialTypeIndex = MaterialManager::getTypeIndex(materialHandle);

		uint32_t shaderTypeIndex = 0;
		MaterialManager::apply(materialHandle, [&shaderTypeIndex](auto& material) {
			shaderTypeIndex =
				((material.template isShaderCompatible<ShaderTypes>() ? getTypeIndex<ShaderTypes>() + 1 : 0) + ...);
		});

		// zero would mean there are no compatible shaders
		assert(shaderTypeIndex != 0);

		// decrement since index was incremented above
		shaderTypeIndex--;

		return shaderTypeIndex;
	}
};


template <typename DerivedManager, typename... ShaderTypes>
std::tuple<std::vector<ShaderTypes>...> ShaderManagerBase<DerivedManager, ShaderTypes...>::shaderObjectArrays {};

template <typename DerivedManager, typename... ShaderTypes>
std::vector<std::vector<typename ShaderManagerBase<DerivedManager, ShaderTypes...>::ShaderInfo>>
	ShaderManagerBase<DerivedManager, ShaderTypes...>::shaderInfoArrays {};

template <typename DerivedManager, typename... ShaderTypes>
std::array<uint32_t, sizeof...(ShaderTypes)> ShaderManagerBase<DerivedManager, ShaderTypes...>::shaderTypeOffsets {};


template <typename DerivedManager, typename... ShaderTypes>
vk::Device ShaderManagerBase<DerivedManager, ShaderTypes...>::vkDevice {};
} // namespace Engine::Managers