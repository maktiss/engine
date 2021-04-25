#pragma once

#include "GraphicsRendererBase.hpp"


namespace Engine {
class ForwardRenderer : public GraphicsRendererBase {
private:
	PROPERTY(uint, "Graphics", clusterCountX, 1);
	PROPERTY(uint, "Graphics", clusterCountY, 1);
	PROPERTY(uint, "Graphics", clusterCountZ, 1);

	PROPERTY(uint, "Graphics", directionalLightCascadeCount, 3);

	PROPERTY(uint, "Graphics", maxVisiblePointLights, 256);
	PROPERTY(uint, "Graphics", maxVisibleSpotLights, 256);

	PROPERTY(float, "Graphics", directionalLightCascadeBase, 2.0f);
	PROPERTY(float, "Graphics", directionalLightCascadeOffset, 0.75f);


	struct EnvironmentBlockMap {
		bool* useDirectionalLight {};
		struct {
			glm::vec3* direction {};

			glm::vec3* color {};
			int32_t* shadowMapIndex {};

			glm::mat4* baseLightSpaceMatrix {};
			glm::mat4* lightSpaceMatrices {};
		} directionalLight {};

		struct LightCluster {
			uint32_t start {};
			uint32_t endShadow {};
			uint32_t end {};

			uint32_t _padding {};
		};

		LightCluster* pointLightClusters {};
		LightCluster* spotLightClusters {};
	};


	struct PointLight {
		glm::vec3 position {};
		float radius {};

		glm::vec3 color {};
		uint32_t shadowMapIndex {};
	};

	struct SpotLight {
		glm::vec3 position {};
		float innerAngle {};

		glm::vec3 direction {};
		float outerAngle {};

		glm::vec3 color {};
		uint shadowMapIndex {};

		glm::mat4 lightSpaceMatrix {};
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

		outputDescriptions[0].format = vk::Format::eR16G16B16A16Sfloat;
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


	inline std::vector<vk::ClearValue> getVkClearValues() override {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(2);

		clearValues[0].color			  = std::array { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1].depthStencil.depth = 1.0f;

		return clearValues;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() override {
		std::vector<vk::AttachmentDescription> attachmentDescriptions {};
		attachmentDescriptions.resize(2);

		auto outputDescriptions = getOutputDescriptions();

		attachmentDescriptions[0].format		 = outputDescriptions[0].format;
		attachmentDescriptions[0].samples		 = vk::SampleCountFlagBits::e1;
		attachmentDescriptions[0].loadOp		 = vk::AttachmentLoadOp::eLoad;
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

	inline std::vector<vk::AttachmentReference> getVkAttachmentReferences() override {
		std::vector<vk::AttachmentReference> attachmentReferences {};
		attachmentReferences.resize(2);

		attachmentReferences[0].attachment = 0;
		attachmentReferences[0].layout	   = vk::ImageLayout::eColorAttachmentOptimal;
		attachmentReferences[1].attachment = 1;
		attachmentReferences[1].layout	   = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return attachmentReferences;
	}


	inline vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() override {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = true;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = false;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eEqual;

		return pipelineDepthStencilStateCreateInfo;
	}
};
} // namespace Engine