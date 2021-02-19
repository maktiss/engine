#pragma once

#include "engine/managers/MeshManager.hpp"
#include "engine/managers/ShaderManager.hpp"
#include "engine/managers/TextureManager.hpp"

#include "engine/graphics/UniqueUniformBuffer.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <vector>


namespace Engine::Renderers {
class RendererBase {
public:
	// TODO: better name?
	struct AttachmentDescription {
		vk::Format format;
		vk::ImageUsageFlags usage;
	};


protected:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	vk::Extent2D outputSize {};

	uint threadCount		 = 1;
	uint framesInFlightCount = 3;

	// number of layers renderer can to render to
	uint layerCount = 1;

	// Created and set by rendering system
	std::vector<Engine::Managers::TextureManager::Handle> inputs {};
	std::vector<Engine::Managers::TextureManager::Handle> outputs {};

	// Set by rendering system based on render graph
	std::vector<vk::ImageLayout> vkInputInitialLayouts {};
	std::vector<vk::ImageLayout> vkOutputInitialLayouts {};
	std::vector<vk::ImageLayout> vkInputFinalLayouts {};
	std::vector<vk::ImageLayout> vkOutputFinalLayouts {};


	vk::PipelineLayout vkPipelineLayout {};
	std::vector<vk::Pipeline> vkPipelines {};

	std::vector<Engine::Graphics::UniqueUniformBuffer> uniformBuffers {};


public:
	RendererBase(uint inputCount, uint outputCount, uint layerCount = 1) : layerCount(layerCount) {
		inputs.resize(inputCount);
		outputs.resize(outputCount);

		vkInputInitialLayouts.resize(inputCount, vk::ImageLayout::eUndefined);
		vkOutputInitialLayouts.resize(outputCount, vk::ImageLayout::eUndefined);

		vkInputFinalLayouts.resize(inputCount, vk::ImageLayout::eGeneral);
		vkOutputFinalLayouts.resize(outputCount, vk::ImageLayout::eGeneral);
	}


	virtual int init() = 0;

	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) = 0;


	virtual std::vector<AttachmentDescription> getOutputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}

	virtual std::vector<AttachmentDescription> getInputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}


	// Returns expected initial layouts, does not apply if resource is not a reference
	virtual std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		return std::vector<vk::ImageLayout>();
	}

	// Returns expected initial layouts, does not apply if resource is not a reference
	virtual std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		return std::vector<vk::ImageLayout>();
	}


	inline uint getInputCount() const {
		return inputs.size();
	}

	inline uint getOutputCount() const {
		return outputs.size();
	}


	inline auto getLayerCount() const {
		return layerCount;
	}


	inline void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	inline void setOutputSize(vk::Extent2D extent) {
		outputSize = extent;
	}

	inline void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}


	inline void setThreadCount(uint count) {
		threadCount = count;
	}

	inline void setFramesInFlightCount(uint count) {
		framesInFlightCount = count;
	}


	inline void setOutput(uint index, Engine::Managers::TextureManager::Handle textureHandle) {
		outputs[index] = textureHandle;
	}

	inline void setInput(uint index, Engine::Managers::TextureManager::Handle textureHandle) {
		inputs[index] = textureHandle;
	}


	inline Engine::Managers::TextureManager::Handle getOutput(uint index) {
		return outputs[index];
	}

	inline Engine::Managers::TextureManager::Handle getInput(uint index) {
		return inputs[index];
	}


	inline void setInputInitialLayout(uint index, vk::ImageLayout imageLayout) {
		vkInputInitialLayouts[index] = imageLayout;
	}

	inline void setOutputInitialLayout(uint index, vk::ImageLayout imageLayout) {
		vkOutputInitialLayouts[index] = imageLayout;
	}

	inline void setInputFinalLayout(uint index, vk::ImageLayout imageLayout) {
		vkInputFinalLayouts[index] = imageLayout;
	}

	inline void setOutputFinalLayout(uint index, vk::ImageLayout imageLayout) {
		vkOutputFinalLayouts[index] = imageLayout;
	}


	const auto getVkDescriptorSetLayouts() {
		std::vector<vk::DescriptorSetLayout> layouts(uniformBuffers.size());
		for (uint i = 0; i < layouts.size(); i++) {
			layouts[i] = uniformBuffers[i].getVkDescriptorSetLayout();
		}
		return layouts;
	}


	void dispose() {
		for (auto& uniformBuffer : uniformBuffers) {
			uniformBuffer.dispose();
		}
	}
};
} // namespace Engine::Renderers