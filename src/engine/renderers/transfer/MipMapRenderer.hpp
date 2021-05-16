#pragma once

#include "TransferRendererBase.hpp"


namespace Engine {
class MipMapRenderer : public TransferRendererBase {
public:
	MipMapRenderer() : TransferRendererBase(0, 1) {
	}


	int init() override;

	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) override;


	virtual std::vector<std::string> getInputNames() const {
		return {};
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "Buffer" };
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const override {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].usage		  = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
		outputDescriptions[0].needMipMaps = true;

		return outputDescriptions;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const override {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eTransferDstOptimal;

		return outputInitialLayouts;
	}
};
} // namespace Engine