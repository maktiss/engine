#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine::Renderers::Graphics {
class ForwardRenderer : public GraphicsRendererBase {
private:
	struct EnvironmentBlock {
		struct {
			alignas(16) glm::vec3 direction;
			alignas(16) glm::vec3 color;
			alignas(4) int32_t shadowMapIndex;
			glm::mat4 lightSpaceMatrices[3]; // FIXME cascade count
		} directionalLight;

		alignas(16) int pointLightCount;
		struct {
			glm::vec3 position;
			float radius;

			glm::vec3 color;
			int shadowMapIndex;

			glm::mat4 lightSpaceMatrices[6];
		} pointLights[8];
	};

public:
	ForwardRenderer() : GraphicsRendererBase(1, 2) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, uint layerIndex,
									   double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_FORWARD";
	}


	std::vector<AttachmentDescription> getOutputDescriptions() const {
		std::vector<AttachmentDescription> outputDescriptions {};
		outputDescriptions.resize(2);

		outputDescriptions[0].format = vk::Format::eR8G8B8A8Srgb;
		outputDescriptions[0].usage	 = vk::ImageUsageFlagBits::eColorAttachment;

		outputDescriptions[1].format = vk::Format::eD24UnormS8Uint;
		outputDescriptions[1].usage	 = vk::ImageUsageFlagBits::eDepthStencilAttachment;

		return outputDescriptions;
	}

	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> descriptions {};
		descriptions.resize(1);

		descriptions[0].format = vk::Format::eD24UnormS8Uint;
		descriptions[0].usage  = vk::ImageUsageFlagBits::eSampled;

		return descriptions;
	}


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(1);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;

		return initialLayouts;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(2);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;
		outputInitialLayouts[1] = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return outputInitialLayouts;
	}


	inline std::vector<vk::ClearValue> getVkClearValues() {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(2);

		clearValues[0].color			  = std::array { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1].depthStencil.depth = 1.0f;

		return clearValues;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(2);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eClear;
		attachmentDescriptions[0].storeOp		 = vk::AttachmentStoreOp::eStore;
		attachmentDescriptions[0].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		attachmentDescriptions[1].format		 = outputDescriptions[1].format;
		attachmentDescriptions[1].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[1].loadOp		 = vk::AttachmentLoadOp::eLoad;
		attachmentDescriptions[1].storeOp		 = vk::AttachmentStoreOp::eDontCare;
		attachmentDescriptions[1].stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
		attachmentDescriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

		return attachmentDescriptions;
	}

	inline std::vector<vk::AttachmentReference> getVkAttachmentReferences() {
		std::vector<vk::AttachmentReference> attachmentReferences {};
		attachmentReferences.resize(2);

		attachmentReferences[0].attachment = 0;
		attachmentReferences[0].layout	   = vk::ImageLayout::eColorAttachmentOptimal;
		attachmentReferences[1].attachment = 1;
		attachmentReferences[1].layout	   = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return attachmentReferences;
	}


	inline vk::PipelineInputAssemblyStateCreateInfo getVkPipelineInputAssemblyStateCreateInfo() {
		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
		pipelineInputAssemblyStateCreateInfo.topology				= vk::PrimitiveTopology::eTriangleList;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

		return pipelineInputAssemblyStateCreateInfo;
	}

	inline vk::Viewport getVkViewport() {
		vk::Viewport viewport {};
		viewport.x		  = 0.0f;
		viewport.y		  = outputSize.height;
		viewport.width	  = outputSize.width;
		viewport.height	  = -(static_cast<int>(outputSize.height));
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		return viewport;
	}

	inline vk::Rect2D getVkScissor() {
		vk::Rect2D scissor {};
		scissor.offset = vk::Offset2D(0, 0);
		scissor.extent = outputSize;

		return scissor;
	}

	inline vk::PipelineRasterizationStateCreateInfo getVkPipelineRasterizationStateCreateInfo() {
		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
		pipelineRasterizationStateCreateInfo.depthClampEnable		 = false;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
		pipelineRasterizationStateCreateInfo.polygonMode			 = vk::PolygonMode::eFill;
		pipelineRasterizationStateCreateInfo.cullMode				 = vk::CullModeFlagBits::eBack;
		pipelineRasterizationStateCreateInfo.frontFace				 = vk::FrontFace::eCounterClockwise;
		pipelineRasterizationStateCreateInfo.depthBiasEnable		 = false;
		pipelineRasterizationStateCreateInfo.lineWidth				 = 1.0f;

		return pipelineRasterizationStateCreateInfo;
	}

	inline vk::PipelineMultisampleStateCreateInfo getVkPipelineMultisampleStateCreateInfo() {
		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
		pipelineMultisampleStateCreateInfo.sampleShadingEnable	= false;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

		return pipelineMultisampleStateCreateInfo;
	}

	inline vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = true;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = false;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eEqual;

		return pipelineDepthStencilStateCreateInfo;
	}

	inline vk::PipelineColorBlendAttachmentState getVkPipelineColorBlendAttachmentState() {
		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
		pipelineColorBlendAttachmentState.colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		pipelineColorBlendAttachmentState.blendEnable = false;

		return pipelineColorBlendAttachmentState;
	}
};
} // namespace Engine::Renderers::Graphics