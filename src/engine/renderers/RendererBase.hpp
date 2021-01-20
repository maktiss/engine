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


	// Created and set by rendering system
	std::vector<Engine::Managers::TextureManager::Handle> inputs {};
	std::vector<Engine::Managers::TextureManager::Handle> outputs {};

	// Set by rendering system based on render graph
	std::vector<vk::ImageLayout> vkOutputInitialLayouts {};
	std::vector<vk::ImageLayout> vkOutputFinalLayouts {};


	vk::PipelineLayout vkPipelineLayout {};
	std::vector<vk::Pipeline> vkPipelines {};

	std::vector<Engine::Graphics::UniqueUniformBuffer> uniformBuffers {};


public:
	RendererBase(uint inputCount, uint outputCount) {
		inputs.resize(inputCount);
		outputs.resize(outputCount);

		vkOutputInitialLayouts.resize(outputCount);
		vkOutputFinalLayouts.resize(outputCount);
	}


	virtual int init() = 0;

	virtual int render(vk::CommandBuffer commandBuffer, double dt) = 0;


	virtual std::vector<AttachmentDescription> getOutputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}

	virtual std::vector<AttachmentDescription> getInputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}


	void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	void setOutputSize(vk::Extent2D extent) {
		outputSize = extent;
	}

	void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}


	void setThreadCount(uint count) {
		threadCount = count;
	}

	void setFramesInFlightCount(uint count) {
		framesInFlightCount = count;
	}


	void setOutput(uint index, Engine::Managers::TextureManager::Handle textureHandle) {
		outputs[index] = textureHandle;
	}

	void setInput(uint index, Engine::Managers::TextureManager::Handle textureHandle) {
		inputs[index] = textureHandle;
	}


	Engine::Managers::TextureManager::Handle getOutput(uint index) {
		return outputs[index];
	}

	Engine::Managers::TextureManager::Handle getInput(uint index) {
		return inputs[index];
	}


	void setOutputInitialLayout(uint index, vk::ImageLayout imageLayout) {
		vkOutputInitialLayouts[index] = imageLayout;
	}

	void setOutputFinalLayout(uint index, vk::ImageLayout imageLayout) {
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