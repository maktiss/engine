#pragma once

#include "GraphicsRendererBase.hpp"

#include "ObjectRenderer.hpp"
#include "TerrainRenderer.hpp"


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


private:
	struct CameraBlock {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;

		glm::mat4 invViewMatrix;
		glm::mat4 invProjectionMatrix;
	} uCameraBlock;


	struct DirectionalLightBlock {
		glm::vec3 direction {};
		bool enabled {};

		glm::vec3 color {};
		int32_t shadowMapIndex {};
	} uDirectionalLightBlock;

	std::vector<glm::mat4> uDirectionalLightMatrices { directionalLightCascadeCount };

	struct TerrainBlock {
		uint size;
		uint maxHeight;

		uint textureHeight;
		uint textureNormal;

		float texelSize;
	} uTerrainBlock;


	struct LightCluster {
		uint32_t start {};
		uint32_t endShadow {};
		uint32_t end {};

		uint32_t _padding {};
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


private:
	ObjectRenderer objectRenderer {};
	TerrainRenderer terrainRenderer {};


public:
	ForwardRenderer() : GraphicsRendererBase(5, 2) {
	}


	int init() override;

	void recordSecondaryCommandBuffers(const vk::CommandBuffer* pSecondaryCommandBuffers, double dt) override;

	const char* getRenderPassName() const override {
		return "RENDER_PASS_FORWARD";
	}


	virtual std::vector<std::string> getInputNames() const {
		return { "ShadowMap", "NormalBuffer", "ReflectionBuffer", "IrradianceMap", "VolumetricLightBuffer" };
	}

	virtual std::vector<std::string> getOutputNames() const {
		return { "ColorBuffer", "DepthBuffer" };
	}


	std::vector<AttachmentDescription> getInputDescriptions() const {
		std::vector<AttachmentDescription> inputDescriptions {};
		inputDescriptions.resize(5);

		inputDescriptions[0].format = vk::Format::eR32G32Sfloat;
		inputDescriptions[0].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[1].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[1].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[2].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[2].usage	= vk::ImageUsageFlagBits::eSampled;

		inputDescriptions[3].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[3].usage	= vk::ImageUsageFlagBits::eSampled;
		inputDescriptions[3].flags	= vk::ImageCreateFlagBits::eCubeCompatible;

		inputDescriptions[4].format = vk::Format::eR16G16B16A16Sfloat;
		inputDescriptions[4].usage	= vk::ImageUsageFlagBits::eSampled;

		return inputDescriptions;
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


	std::vector<vk::ImageLayout> getInputInitialLayouts() const {
		std::vector<vk::ImageLayout> initialLayouts {};
		initialLayouts.resize(5);

		initialLayouts[0] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[1] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[2] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[3] = vk::ImageLayout::eShaderReadOnlyOptimal;
		initialLayouts[4] = vk::ImageLayout::eShaderReadOnlyOptimal;

		return initialLayouts;
	}

	std::vector<vk::ImageLayout> getOutputInitialLayouts() const {
		std::vector<vk::ImageLayout> outputInitialLayouts {};
		outputInitialLayouts.resize(2);

		outputInitialLayouts[0] = vk::ImageLayout::eColorAttachmentOptimal;
		outputInitialLayouts[1] = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		return outputInitialLayouts;
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


	std::vector<vk::ImageViewCreateInfo> getInputVkImageViewCreateInfos() override {
		auto imageViewCreateInfos = RendererBase::getInputVkImageViewCreateInfos();

		{
			auto& imageViewCreateInfo	 = imageViewCreateInfos[getInputIndex("ShadowMap")];
			imageViewCreateInfo.viewType = vk::ImageViewType::e2DArray;
		}

		{
			auto& imageViewCreateInfo	 = imageViewCreateInfos[getInputIndex("IrradianceMap")];
			imageViewCreateInfo.viewType = vk::ImageViewType::eCube;
		}

		return imageViewCreateInfos;
	}


	std::vector<DescriptorSetDescription> getDescriptorSetDescriptions() const {
		std::vector<DescriptorSetDescription> descriptorSetDescriptions {};

		descriptorSetDescriptions.push_back({ 0, 0, vk::DescriptorType::eUniformBuffer, sizeof(CameraBlock) });

		uint directionalLightMatricesBlockSize = directionalLightCascadeCount * sizeof(glm::mat4);
		uint pointLightsBlockSize			   = maxVisiblePointLights * sizeof(PointLight);
		uint spotLightsBlockSize			   = maxVisibleSpotLights * sizeof(SpotLight);

		descriptorSetDescriptions.push_back(
			{ 1, 0, vk::DescriptorType::eUniformBuffer, sizeof(DirectionalLightBlock) });
		descriptorSetDescriptions.push_back(
			{ 1, 1, vk::DescriptorType::eUniformBuffer, directionalLightMatricesBlockSize });
		descriptorSetDescriptions.push_back({ 1, 2, vk::DescriptorType::eUniformBuffer, 4 }); // FIXME
		descriptorSetDescriptions.push_back({ 1, 3, vk::DescriptorType::eUniformBuffer, 4 }); // FIXME
		descriptorSetDescriptions.push_back({ 1, 4, vk::DescriptorType::eStorageBuffer, pointLightsBlockSize });
		descriptorSetDescriptions.push_back({ 1, 5, vk::DescriptorType::eStorageBuffer, spotLightsBlockSize });

		descriptorSetDescriptions.push_back({ 2, 0, vk::DescriptorType::eUniformBuffer, sizeof(TerrainBlock) });

		return descriptorSetDescriptions;
	}


	std::vector<SpecializationConstantDescription> getSpecializationConstantDescriptions() const override {
		std::vector<SpecializationConstantDescription> specializationConstantDescriptions {};
		specializationConstantDescriptions.resize(3);

		specializationConstantDescriptions[0] = { 100, (void*)(&directionalLightCascadeCount),
												  sizeof(directionalLightCascadeCount) };
		specializationConstantDescriptions[1] = { 101, (void*)(&directionalLightCascadeBase),
												  sizeof(directionalLightCascadeBase) };
		specializationConstantDescriptions[2] = { 102, (void*)(&directionalLightCascadeOffset),
												  sizeof(directionalLightCascadeOffset) };

		return specializationConstantDescriptions;
	}


	inline std::vector<vk::ClearValue> getVkClearValues() const override {
		std::vector<vk::ClearValue> clearValues {};
		clearValues.resize(2);

		clearValues[0].color			  = std::array { 0.0f, 0.0f, 0.0f, 0.0f };
		clearValues[1].depthStencil.depth = 1.0f;

		return clearValues;
	}


	inline std::vector<vk::AttachmentDescription> getVkAttachmentDescriptions() const override {
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


	// vk::PipelineRasterizationStateCreateInfo getVkPipelineRasterizationStateCreateInfo() const {
	// 	vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
	// 	pipelineRasterizationStateCreateInfo.depthClampEnable		 = false;
	// 	pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
	// 	pipelineRasterizationStateCreateInfo.polygonMode			 = vk::PolygonMode::eLine;
	// 	pipelineRasterizationStateCreateInfo.cullMode				 = vk::CullModeFlagBits::eNone;
	// 	pipelineRasterizationStateCreateInfo.frontFace				 = vk::FrontFace::eCounterClockwise;
	// 	pipelineRasterizationStateCreateInfo.depthBiasEnable		 = false;
	// 	pipelineRasterizationStateCreateInfo.lineWidth				 = 1.0f;

	// 	return pipelineRasterizationStateCreateInfo;
	// }


	inline vk::PipelineDepthStencilStateCreateInfo getVkPipelineDepthStencilStateCreateInfo() const override {
		vk::PipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo {};
		pipelineDepthStencilStateCreateInfo.depthTestEnable	 = true;
		pipelineDepthStencilStateCreateInfo.depthWriteEnable = false;
		pipelineDepthStencilStateCreateInfo.depthCompareOp	 = vk::CompareOp::eEqual;

		return pipelineDepthStencilStateCreateInfo;
	}
};
} // namespace Engine