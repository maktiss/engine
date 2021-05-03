#pragma once

#include "TransferRendererBase.hpp"


namespace Engine {
class MipMapRenderer : public TransferRendererBase {
public:
	MipMapRenderer() : TransferRendererBase(0, 1) {
	}


	int init() override;

	virtual void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
											   double dt) override;


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(1);

		outputDescriptions[0].usage = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
		outputDescriptions[0].needMipMaps = true;

		return outputDescriptions;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(1);

		outputInitialLayouts[0] = vk::ImageLayout::eTransferDstOptimal;

		return outputInitialLayouts;
	}
};
} // namespace Engine