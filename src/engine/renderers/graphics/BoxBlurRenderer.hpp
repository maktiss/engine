#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class BoxBlurRenderer : public GraphicsRendererBase {
private:
	MeshManager::Handle mesh {};
	GraphicsShaderManager::Handle shaderHandle {};

	bool blurDirection {};
	uint32_t blurKernelSize = 5;

	vk::Format outputFormat {};


public:
	BoxBlurRenderer() : GraphicsRendererBase(1, 1) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_BOX_BLUR";
	}


	virtual std::vector<std::string> getInputNames() const {
		return { "ColorBuffer" };
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "ColorBuffer" };
	}


	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> descriptions {};
		descriptions.resize(1);

		descriptions[0].format = vk::Format::eR32G32Sfloat;
		descriptions[0].usage  = vk::ImageUsageFlagBits::eSampled;

		return descriptions;
	}

	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].format = outputFormat;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		return outputDescriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(1);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;

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
			auto& imageViewCreateInfo	 = imageViewCreateInfos[getInputIndex("ColorBuffer")];
			imageViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
		}

		return imageViewCreateInfos;
	}


	std::vector<DescriptorSetDescription> getDescriptorSetDescriptions() const {
		std::vector<DescriptorSetDescription> descriptorSetDescriptions {};

		return descriptorSetDescriptions;
	}


	std::vector<SpecializationConstantDescription> getSpecializationConstantDescriptions() const override {
		std::vector<SpecializationConstantDescription> specializationConstantDescriptions {};
		specializationConstantDescriptions.resize(2);

		specializationConstantDescriptions[0] = { 0, (void*)(&blurDirection), sizeof(blurDirection) };
		specializationConstantDescriptions[1] = { 1, (void*)(&blurKernelSize), sizeof(blurKernelSize) };

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


	inline void setDirection(bool direction) {
		blurDirection = direction;
	}

	inline void setKernelSize(uint32_t kernelSize) {
		blurKernelSize = kernelSize;
	}

	inline void setOutputFormat(vk::Format format) {
		outputFormat = format;
	}
};
} // namespace Engine