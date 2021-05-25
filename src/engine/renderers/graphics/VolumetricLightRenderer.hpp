#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class VolumetricLightRenderer : public GraphicsRendererBase {
private:
	PROPERTY(uint, "Graphics", directionalLightCascadeCount, 3);

	PROPERTY(float, "Graphics", directionalLightCascadeBase, 2.0f);
	PROPERTY(float, "Graphics", directionalLightCascadeOffset, 0.75f);

	PROPERTY(uint, "Graphics", volumetricLightSampleCount, 64);


private:
	struct CameraBlock {
		glm::mat4 invViewMatrix {};
		glm::mat4 invProjectionMatrix {};
	};

	std::vector<glm::mat4> uDirectionalLightMatrices { directionalLightCascadeCount };


private:
	MeshManager::Handle mesh {};
	GraphicsShaderManager::Handle shaderHandle {};


public:
	VolumetricLightRenderer() : GraphicsRendererBase(2, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_VOLUMETRIC_LIGHT";
	}


	virtual std::vector<std::string> getInputNames() const {
		return { "DepthBuffer", "ShadowMap" };
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "VolumetricLightBuffer" };
	}


	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> inputDescriptions {};
		inputDescriptions.resize(2);

		inputDescriptions[0].format = vk::Format::eD24UnormS8Uint;
		inputDescriptions[0].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[1].format = vk::Format::eR32G32Sfloat;
		inputDescriptions[1].usage	= vk::ImageUsageFlagBits::eSampled;

		return inputDescriptions;
	}

	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(2);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[1] = vk::ImageLayout::eShaderReadOnlyOptimal;

		return initialLayouts;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;

		return outputInitialLayouts;
	}


	std::vector<vk::ImageViewCreateInfo> getInputVkImageViewCreateInfos() override {
		auto imageViewCreateInfos = RendererBase::getInputVkImageViewCreateInfos();

		{
			auto& imageViewCreateInfo						= imageViewCreateInfos[getInputIndex("DepthBuffer")];
			imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
		}

		{
			auto& imageViewCreateInfo	 = imageViewCreateInfos[getInputIndex("ShadowMap")];
			imageViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
		}

		return imageViewCreateInfos;
	}


	std::vector<vk::SamplerCreateInfo> getInputVkSamplerCreateInfos() override {
		auto samplerCreateInfos = RendererBase::getInputVkSamplerCreateInfos();

		{
			auto& samplerCreateInfo		   = samplerCreateInfos[getInputIndex("ShadowMap")];
			samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToBorder;
			samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToBorder;
			samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToBorder;
			samplerCreateInfo.borderColor  = vk::BorderColor::eFloatOpaqueWhite;
		}

		return samplerCreateInfos;
	}


	std::vector<DescriptorSetDescription> getDescriptorSetDescriptions() const {
		std::vector<DescriptorSetDescription> descriptorSetDescriptions {};

		descriptorSetDescriptions.push_back({ 0, 0, vk::DescriptorType::eUniformBuffer, sizeof(CameraBlock) });

		uint directionalLightMatricesBlockSize = directionalLightCascadeCount * sizeof(glm::mat4);

		descriptorSetDescriptions.push_back(
			{ 1, 0, vk::DescriptorType::eUniformBuffer, directionalLightMatricesBlockSize });

		descriptorSetDescriptions.push_back({ 1, 1, vk::DescriptorType::eUniformBuffer, 12 });

		return descriptorSetDescriptions;
	}


	std::vector<SpecializationConstantDescription> getSpecializationConstantDescriptions() const override {
		std::vector<SpecializationConstantDescription> specializationConstantDescriptions {};

		specializationConstantDescriptions.push_back(
			{ 0, (void*)(&volumetricLightSampleCount), sizeof(volumetricLightSampleCount) });

		return specializationConstantDescriptions;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(1);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		return attachmentDescriptions;
	}
};
} // namespace Engine