#pragma once

#include "engine/managers/Managers.hpp"

#include "engine/graphics/DescriptorSetArray.hpp"

#define VULKAN_HPP_NO_EXCEPTIONS 1
#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"

#include <string>
#include <vector>


namespace Engine {
class RendererBase {
public:
	// TODO: better name?
	struct AttachmentDescription {
		vk::Format format {};

		vk::ImageUsageFlags usage {};
		vk::ImageCreateFlags flags {};

		bool needMipMaps {};

		// TODO: required attachment flag
		// bool required {};
	};

	struct SpecializationConstantDescription {
		uint id {};

		void* pData {};
		size_t size {};
	};

	struct DescriptorSetDescription {
		uint setIndex {};
		uint bindingIndex {};

		vk::DescriptorType type {};

		uint64_t size {};
		uint descriptorCount = 1;
	};


protected:
	vk::Device vkDevice {};
	VmaAllocator vmaAllocator {};

	vk::Extent2D outputSize {};

	uint layerCount = 1;

	uint threadCount		 = 1;
	uint framesInFlightCount = 3;

	uint currentFrameInFlight {};

	// Created and set by rendering system
	std::vector<TextureManager::Handle> inputs {};
	std::vector<TextureManager::Handle> outputs {};

	// Set by rendering system, used for log messages
	std::string rendererName {};

	// Set by rendering system based on render graph
	std::vector<vk::ImageLayout> vkInputInitialLayouts {};
	std::vector<vk::ImageLayout> vkOutputInitialLayouts {};
	std::vector<vk::ImageLayout> vkInputFinalLayouts {};
	std::vector<vk::ImageLayout> vkOutputFinalLayouts {};


	vk::PipelineLayout vkPipelineLayout {};
	std::vector<vk::Pipeline> vkPipelines {};

	std::vector<vk::Sampler> inputVkSamplers {};
	std::vector<vk::ImageView> inputVkImageViews {};

	std::vector<DescriptorSetArray> descriptorSetArrays {};


public:
	RendererBase(uint inputCount, uint outputCount) {
		inputs.resize(inputCount);
		outputs.resize(outputCount);

		vkInputInitialLayouts.resize(inputCount, vk::ImageLayout::eUndefined);
		vkOutputInitialLayouts.resize(outputCount, vk::ImageLayout::eUndefined);

		vkInputFinalLayouts.resize(inputCount, vk::ImageLayout::eGeneral);
		vkOutputFinalLayouts.resize(outputCount, vk::ImageLayout::eGeneral);
	}


	virtual int init();

	virtual int render(const vk::CommandBuffer* pPrimaryCommandBuffers,
					   const vk::CommandBuffer* pSecondaryCommandBuffers, const vk::QueryPool& timestampQueryPool,
					   double dt) = 0;


	inline uint getInputCount() const {
		return inputs.size();
	}

	inline uint getOutputCount() const {
		return outputs.size();
	}


	inline uint getLayerCount() const {
		return layerCount;
	}

	inline void setLayerCount(uint count) {
		layerCount = count;
	}


	virtual std::vector<AttachmentDescription> getInputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}

	virtual std::vector<AttachmentDescription> getOutputDescriptions() const {
		return std::vector<AttachmentDescription>();
	}


	virtual std::vector<std::string> getInputNames() const {
		return std::vector<std::string>();
	}

	virtual std::vector<std::string> getOutputNames() const {
		return std::vector<std::string>();
	}


	inline uint getInputIndex(std::string name) {
		const auto inputNames = getInputNames();
		auto iter			  = std::find(inputNames.begin(), inputNames.end(), name);

		assert(iter != inputNames.end());

		return std::distance(inputNames.begin(), iter);
	}

	inline uint getOutputIndex(std::string name) {
		const auto outputNames = getOutputNames();
		auto iter			   = std::find(outputNames.begin(), outputNames.end(), name);

		assert(iter != outputNames.end());

		return std::distance(outputNames.begin(), iter);
	}


	// Returns expected initial layouts, does not apply if resource is not a reference
	virtual std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		return std::vector<vk::ImageLayout>();
	}

	// Returns expected initial layouts, does not apply if resource is not a reference
	virtual std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		return std::vector<vk::ImageLayout>();
	}


	// Number of multiview layers per layer
	virtual inline uint getMultiviewLayerCount() const {
		return 1;
	}


	inline void setVkDevice(vk::Device device) {
		vkDevice = device;
	}

	inline void setVulkanMemoryAllocator(VmaAllocator allocator) {
		vmaAllocator = allocator;
	}


	inline void setOutputSize(vk::Extent2D extent) {
		outputSize = extent;
	}

	inline auto getOutputSize() const {
		return outputSize;
	}


	inline void setThreadCount(uint count) {
		threadCount = count;
	}

	inline void setFramesInFlightCount(uint count) {
		framesInFlightCount = count;
	}


	inline void setOutput(uint index, TextureManager::Handle textureHandle) {
		outputs[index] = textureHandle;
	}

	inline void setInput(uint index, TextureManager::Handle textureHandle) {
		inputs[index] = textureHandle;
	}


	inline TextureManager::Handle getOutput(uint index) const {
		return outputs[index];
	}

	inline TextureManager::Handle getInput(uint index) const {
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


	inline void setRendererName(std::string name) {
		rendererName = name;
	}


	void dispose();


protected:
	inline virtual std::vector<vk::SamplerCreateInfo> getInputVkSamplerCreateInfos() {
		std::vector<vk::SamplerCreateInfo> samplerCreateInfos {};

		vk::SamplerCreateInfo samplerCreateInfo {};
		samplerCreateInfo.minFilter				  = vk::Filter::eLinear;
		samplerCreateInfo.magFilter				  = vk::Filter::eLinear;
		samplerCreateInfo.addressModeU			  = vk::SamplerAddressMode::eClampToEdge;
		samplerCreateInfo.addressModeV			  = vk::SamplerAddressMode::eClampToEdge;
		samplerCreateInfo.addressModeW			  = vk::SamplerAddressMode::eClampToEdge;
		samplerCreateInfo.anisotropyEnable		  = false;
		samplerCreateInfo.maxAnisotropy			  = 0.0f;
		samplerCreateInfo.unnormalizedCoordinates = false;
		samplerCreateInfo.compareEnable			  = false;
		samplerCreateInfo.compareOp				  = vk::CompareOp::eAlways;
		samplerCreateInfo.mipmapMode			  = vk::SamplerMipmapMode::eLinear;
		samplerCreateInfo.mipLodBias			  = 0.0f;
		samplerCreateInfo.minLod				  = 0.0f;
		samplerCreateInfo.maxLod				  = VK_LOD_CLAMP_NONE;

		samplerCreateInfos.resize(getInputCount(), samplerCreateInfo);

		return samplerCreateInfos;
	}

	inline virtual std::vector<vk::ImageViewCreateInfo> getInputVkImageViewCreateInfos() {
		std::vector<vk::ImageViewCreateInfo> imageViewCreateInfos {};

		for (uint inputIndex = 0; inputIndex < getInputCount(); inputIndex++) {
			vk::ImageViewCreateInfo imageViewCreateInfo {};

			auto textureInfo = TextureManager::getTextureInfo(inputs[inputIndex]);

			imageViewCreateInfo.viewType						= vk::ImageViewType::e2D;
			imageViewCreateInfo.format							= textureInfo.format;
			imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
			imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
			imageViewCreateInfo.subresourceRange.levelCount		= textureInfo.mipLevels;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount		= textureInfo.arrayLayers;
			imageViewCreateInfo.image							= textureInfo.image;

			imageViewCreateInfos.push_back(imageViewCreateInfo);
		}

		return imageViewCreateInfos;
	}


	inline virtual std::vector<DescriptorSetDescription> getDescriptorSetDescriptions() const {
		std::vector<DescriptorSetDescription> descriptorSetDescriptions {};
		return descriptorSetDescriptions;
	}


	virtual std::vector<vk::DescriptorSetLayout> getVkDescriptorSetLayouts() {
		std::vector<vk::DescriptorSetLayout> layouts(descriptorSetArrays.size());

		for (uint i = 0; i < layouts.size(); i++) {
			layouts[i] = descriptorSetArrays[i].getVkDescriptorSetLayout();
		}

		layouts.push_back(TextureManager::getVkDescriptorSetLayout());
		layouts.push_back(MaterialManager::getVkDescriptorSetLayout());

		return layouts;
	}


	virtual std::vector<SpecializationConstantDescription> getSpecializationConstantDescriptions() const {
		return std::vector<SpecializationConstantDescription>();
	}


	inline int testVkResult(vk::Result result, std::string errorMessage) {
		if (result != vk::Result::eSuccess) {
			if (errorMessage.empty()) {
				errorMessage = "Vulkan error occured";
			}
			spdlog::error("[{}] {}. Error code: {} ({})", rendererName, errorMessage, result,
						  vk::to_string(vk::Result(result)));
			return VkResult(result);
		}
		return 0;
	}


	void bindDescriptorSets(const vk::CommandBuffer& commandBuffer, vk::PipelineBindPoint bindPoint, uint elementIndex);


private:
	int defineDescriptorSets();

	int createVkPipelineLayout();

	int createInputVkSamplers();
	int createInputVkImageViews();
};
} // namespace Engine