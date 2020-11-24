#include "RenderingSystem.hpp"


namespace Engine::Systems {
int RenderingSystem::init() {
	spdlog::info("RenderingSystem: Initialization...");


	// add required glfw extensions

	uint32_t extensionCount					 = 0;
	const char** requiredGLFWExtensionsNames = glfwGetRequiredInstanceExtensions(&extensionCount);

	std::vector<const char*> requiredExtensionNames(requiredGLFWExtensionsNames,
													requiredGLFWExtensionsNames + extensionCount);

	addRequiredInstanceExtensionNames(requiredExtensionNames);


	if (initVkInstance()) {
		return 1;
	}


	// setup vulkan surface

	auto pVkSurface = VkSurfaceKHR(vkSurface);
	auto result		= glfwCreateWindowSurface(VkInstance(vkInstance), glfwWindow, nullptr, &pVkSurface);
	if (result) {
		spdlog::error("Failed to create Vulkan surface. Error code: {}", result);
		return 1;
	}
	vkSurface = vk::SurfaceKHR(pVkSurface);


	if (enumeratePhysicalDevices()) {
		return 1;
	}

	if (createLogicalDevice()) {
		return 1;
	}

	if (createSwapchain()) {
		return 1;
	}


	// FIXME testing

	{
		if (Engine::Utils::readFile("assets/shaders/triangle.vsh.spv", vshCode)) {
			spdlog::error("Failed to read triangle.vsh.spv");
			return 1;
		}
		if (Engine::Utils::readFile("assets/shaders/triangle.fsh.spv", fshCode)) {
			spdlog::error("Failed to read triangle.fsh.spv");
			return 1;
		}

		spdlog::info("{}, {}", vshCode.size(), fshCode.size());

		vk::ShaderModule vsShaderModule;
		vk::ShaderModule fsShaderModule;

		vk::ShaderModuleCreateInfo shaderModuleCreateInfo {};
		shaderModuleCreateInfo.codeSize = vshCode.size();
		shaderModuleCreateInfo.pCode	= reinterpret_cast<const uint32_t*>(vshCode.data());

		RETURN_IF_VK_ERROR(vkDevice.createShaderModule(&shaderModuleCreateInfo, nullptr, &vsShaderModule),
						   "Failed to create shader module");

		shaderModuleCreateInfo.codeSize = fshCode.size();
		shaderModuleCreateInfo.pCode	= reinterpret_cast<const uint32_t*>(fshCode.data());

		RETURN_IF_VK_ERROR(vkDevice.createShaderModule(&shaderModuleCreateInfo, nullptr, &fsShaderModule),
						   "Failed to create shader module");

		vk::PipelineShaderStageCreateInfo vsPipelineShaderStageCreateInfo {};
		vsPipelineShaderStageCreateInfo.stage  = vk::ShaderStageFlagBits::eVertex;
		vsPipelineShaderStageCreateInfo.module = vsShaderModule;
		vsPipelineShaderStageCreateInfo.pName  = "main";

		vk::PipelineShaderStageCreateInfo fsPipelineShaderStageCreateInfo {};
		fsPipelineShaderStageCreateInfo.stage  = vk::ShaderStageFlagBits::eFragment;
		fsPipelineShaderStageCreateInfo.module = fsShaderModule;
		fsPipelineShaderStageCreateInfo.pName  = "main";

		vk::PipelineShaderStageCreateInfo shaderStages[] = { vsPipelineShaderStageCreateInfo,
															 fsPipelineShaderStageCreateInfo };

		vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};
		pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 0;
		pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount   = 0;

		vk::PipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo {};
		pipelineInputAssemblyStateCreateInfo.topology				= vk::PrimitiveTopology::eTriangleList;
		pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = false;

		vk::Viewport viewport {};
		viewport.x		  = 0.0f;
		viewport.y		  = 0.0f;
		viewport.width	  = vkSwapchainInfo.extent.width;
		viewport.height	  = vkSwapchainInfo.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor {};
		scissor.offset = vk::Offset2D(0, 0);
		scissor.extent = vkSwapchainInfo.extent;

		vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
		pipelineViewportStateCreateInfo.viewportCount = 1;
		pipelineViewportStateCreateInfo.pViewports	  = &viewport;
		pipelineViewportStateCreateInfo.scissorCount  = 1;
		pipelineViewportStateCreateInfo.pScissors	  = &scissor;

		vk::PipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo {};
		pipelineRasterizationStateCreateInfo.depthClampEnable		 = false;
		pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = false;
		pipelineRasterizationStateCreateInfo.polygonMode			 = vk::PolygonMode::eFill;
		pipelineRasterizationStateCreateInfo.cullMode				 = vk::CullModeFlagBits::eBack;
		pipelineRasterizationStateCreateInfo.frontFace				 = vk::FrontFace::eClockwise;
		pipelineRasterizationStateCreateInfo.depthBiasEnable		 = false;
		pipelineRasterizationStateCreateInfo.lineWidth				 = 1.0f;

		vk::PipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo {};
		pipelineMultisampleStateCreateInfo.sampleShadingEnable	= false;
		pipelineMultisampleStateCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vk::PipelineColorBlendAttachmentState pipelineColorBlendAttachmentState {};
		pipelineColorBlendAttachmentState.colorWriteMask =
			vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA;
		pipelineColorBlendAttachmentState.blendEnable = false;

		vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
		pipelineColorBlendStateCreateInfo.logicOpEnable	  = false;
		pipelineColorBlendStateCreateInfo.attachmentCount = 1;
		pipelineColorBlendStateCreateInfo.pAttachments	  = &pipelineColorBlendAttachmentState;

		vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};

		RETURN_IF_VK_ERROR(vkDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &vkPipelineLayout),
						   "Failed to create pipeline layout");

		vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
		graphicsPipelineCreateInfo.stageCount = 2;
		graphicsPipelineCreateInfo.pStages	  = shaderStages;

		graphicsPipelineCreateInfo.pVertexInputState   = &pipelineVertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pViewportState	   = &pipelineViewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState   = &pipelineMultisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState  = nullptr;
		graphicsPipelineCreateInfo.pColorBlendState	   = &pipelineColorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState	   = nullptr;

		graphicsPipelineCreateInfo.layout = vkPipelineLayout;

		graphicsPipelineCreateInfo.renderPass = vkRenderPass;
		graphicsPipelineCreateInfo.subpass	  = 0;

		RETURN_IF_VK_ERROR(
			vkDevice.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &vkPipeline),
			"Failed to create graphics pipeline");
	}

	return 0;
}

int RenderingSystem::run(double dt) {
	// record command buffers

	for (uint i = 0; i < vkCommandBuffers.size(); i++) {
		auto& commandBuffer = vkCommandBuffers[i];

		vk::CommandBufferBeginInfo commandBufferBeginInfo {};

		RETURN_IF_VK_ERROR(commandBuffer.begin(&commandBufferBeginInfo), "Failed to record command buffer");

		vk::RenderPassBeginInfo renderPassBeginInfo {};
		renderPassBeginInfo.renderPass		  = vkRenderPass;
		renderPassBeginInfo.framebuffer		  = vkSwapchainInfo.framebuffers[i];
		renderPassBeginInfo.renderArea.extent = vkSwapchainInfo.extent;
		renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);

		vk::ClearValue clearValue = { std::array<float, 4> { 0.0f, 0.0f, 0.0f, 1.0f } };

		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues	= &clearValue;


		commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline);

		commandBuffer.draw(3, 1, 0, 0);

		commandBuffer.endRenderPass();


		RETURN_IF_VK_ERROR(commandBuffer.end(), "Failed to record command buffer");
	}

	if (present()) {
		return 1;
	}

	return 0;
}


void RenderingSystem::setWindow(GLFWwindow* pGLFWWindow) {
	glfwWindow = pGLFWWindow;
}


int RenderingSystem::initVkInstance() {
	// TODO: not hardcoded appInfo
	vk::ApplicationInfo appInfo {};
	appInfo.pApplicationName   = "App";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.pEngineName		   = "Engine";
	appInfo.engineVersion	   = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion		   = VK_API_VERSION_1_2;


	vk::InstanceCreateInfo instanceCreateInfo {};
	instanceCreateInfo.pApplicationInfo		   = &appInfo;
	instanceCreateInfo.enabledExtensionCount   = requiredInstanceExtensionNames.size();
	instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensionNames.data();

	// TODO: ability to disable validation layers
	if (true) {
		instanceCreateInfo.enabledLayerCount   = validationLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();
	}

	RETURN_IF_VK_ERROR(vk::createInstance(&instanceCreateInfo, nullptr, &vkInstance), "Failed to init Vulkan instance");

	return 0;
}

int RenderingSystem::enumeratePhysicalDevices() {
	uint32_t deviceCount = 0;
	RETURN_IF_VK_ERROR(vkInstance.enumeratePhysicalDevices(&deviceCount, nullptr),
					   "Failed to enumerate physical devices");

	if (deviceCount == 0) {
		spdlog::error("No compatible GPUs have been found");
		return 1;
	}

	std::vector<vk::PhysicalDevice> physicalDevices(deviceCount);
	RETURN_IF_VK_ERROR(vkInstance.enumeratePhysicalDevices(&deviceCount, physicalDevices.data()),
					   "Failed to enumerate physical devices");

	for (auto physicalDevice : physicalDevices) {
		if (isPhysicalDeviceSupported(physicalDevice)) {
			vkSupportedPhysicalDevices.push_back(physicalDevice);
		}
	}

	if (vkSupportedPhysicalDevices.empty()) {
		spdlog::error("No compatible GPUs have been found");
		return 1;
	}

	std::string logMessage =
		std::to_string(vkSupportedPhysicalDevices.size()) + " compatible device(s) have been found:";
	for (auto physicalDevice : vkSupportedPhysicalDevices) {
		vk::PhysicalDeviceProperties physicalDeviceProperties;
		physicalDevice.getProperties(&physicalDeviceProperties);

		logMessage += "\n\t";
		logMessage += physicalDeviceProperties.deviceName.begin();
		logMessage += " | Vulkan version: ";
		logMessage += std::to_string(VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion));
		logMessage += ".";
		logMessage += std::to_string(VK_VERSION_MINOR(physicalDeviceProperties.apiVersion));
		logMessage += ".";
		logMessage += std::to_string(VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
	}
	spdlog::info(logMessage);

	return 0;
}

int RenderingSystem::createLogicalDevice() {
	auto queueFamilies = getQueueFamilies(getActivePhysicalDevice());

	std::vector<uint32_t> queueFamilyIndices;
	if (queueFamilies.graphicsFamily == queueFamilies.presentFamily) {
		queueFamilyIndices = { queueFamilies.graphicsFamily };
	} else {
		queueFamilyIndices = { queueFamilies.graphicsFamily, queueFamilies.presentFamily };
	}

	std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos(queueFamilyIndices.size());
	for (uint i = 0; i < queueFamilyIndices.size(); i++) {
		auto& deviceQueueCreateInfo			   = deviceQueueCreateInfos[i];
		deviceQueueCreateInfo.queueCount	   = 1;
		deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndices[i];

		float queuePriority					   = 1.0f;
		deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
	}

	vk::PhysicalDeviceFeatures physicalDeviceFeatures {};

	vk::DeviceCreateInfo deviceCreateInfo {};
	deviceCreateInfo.queueCreateInfoCount = deviceQueueCreateInfos.size();
	deviceCreateInfo.pQueueCreateInfos	  = deviceQueueCreateInfos.data();

	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	deviceCreateInfo.enabledExtensionCount	 = requiredDeviceExtensionNames.size();
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtensionNames.data();

	RETURN_IF_VK_ERROR(getActivePhysicalDevice().createDevice(&deviceCreateInfo, nullptr, &vkDevice),
					   "Failed to create Vulkan device");

	vkDevice.getQueue(queueFamilies.graphicsFamily, 0, &vkGraphicsQueue);
	vkDevice.getQueue(queueFamilies.presentFamily, 0, &vkPresentQueue);

	return 0;
}


int RenderingSystem::createSwapchain() {
	auto swapchainSupportInfo = getSwapchainSupportInfo(getActivePhysicalDevice());

	// TODO: choose the best format
	auto surfaceFormat = swapchainSupportInfo.formats[0];
	auto presentMode   = vk::PresentModeKHR::eImmediate;

	auto extent = swapchainSupportInfo.capabilities.maxImageExtent;

	// + 1 image for buffering, make sure it doesn't exceed limit
	auto imageCount =
		std::min(swapchainSupportInfo.capabilities.minImageCount + 1, swapchainSupportInfo.capabilities.maxImageCount);

	vk::SwapchainCreateInfoKHR swapchainCreateInfo {};
	swapchainCreateInfo.surface			 = vkSurface;
	swapchainCreateInfo.minImageCount	 = imageCount;
	swapchainCreateInfo.imageFormat		 = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace	 = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent		 = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage		 = vk::ImageUsageFlagBits::eColorAttachment;

	auto queueFamilies			  = getQueueFamilies(getActivePhysicalDevice());
	uint32_t queueFamilyIndices[] = { queueFamilies.graphicsFamily, queueFamilies.presentFamily };
	if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
		swapchainCreateInfo.imageSharingMode	  = vk::SharingMode::eConcurrent;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices	  = queueFamilyIndices;
	} else {
		swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	swapchainCreateInfo.preTransform   = swapchainSupportInfo.capabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped		= true;

	RETURN_IF_VK_ERROR(vkDevice.createSwapchainKHR(&swapchainCreateInfo, nullptr, &vkSwapchainInfo.swapchain),
					   "Failed to create Vulkan swapchain");


	// retrieve swapchain images

	uint32_t swapchainImageCount = 0;
	RETURN_IF_VK_ERROR(vkDevice.getSwapchainImagesKHR(vkSwapchainInfo.swapchain, &swapchainImageCount, nullptr),
					   "Failed to retrieve swapchain images");
	vkSwapchainInfo.images.resize(swapchainImageCount);
	RETURN_IF_VK_ERROR(
		vkDevice.getSwapchainImagesKHR(vkSwapchainInfo.swapchain, &swapchainImageCount, vkSwapchainInfo.images.data()),
		"Failed to retrieve swapchain images");

	vkSwapchainInfo.imageFormat = surfaceFormat.format;
	vkSwapchainInfo.extent		= extent;


	// create swapchain image views

	vkSwapchainInfo.imageViews.resize(swapchainImageCount);
	for (uint i = 0; i < vkSwapchainInfo.imageViews.size(); i++) {
		vk::ImageViewCreateInfo imageViewCreateInfo {};
		imageViewCreateInfo.image = vkSwapchainInfo.images[i];

		imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imageViewCreateInfo.format	 = vkSwapchainInfo.imageFormat;

		imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;

		imageViewCreateInfo.subresourceRange.aspectMask		= vk::ImageAspectFlagBits::eColor;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.baseMipLevel	= 0;
		imageViewCreateInfo.subresourceRange.layerCount		= 1;
		imageViewCreateInfo.subresourceRange.levelCount		= 1;

		RETURN_IF_VK_ERROR(vkDevice.createImageView(&imageViewCreateInfo, nullptr, &vkSwapchainInfo.imageViews[i]),
						   "Failed to create swapchain image view");
	}


	// TODO: renderers should have their own render passes
	// create render pass

	vk::AttachmentDescription attachmentDescription {};
	attachmentDescription.format  = vkSwapchainInfo.imageFormat;
	attachmentDescription.samples = vk::SampleCountFlagBits::e1;

	attachmentDescription.loadOp  = vk::AttachmentLoadOp::eClear;
	attachmentDescription.storeOp = vk::AttachmentStoreOp::eStore;

	attachmentDescription.stencilLoadOp	 = vk::AttachmentLoadOp::eDontCare;
	attachmentDescription.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

	attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
	attachmentDescription.finalLayout	= vk::ImageLayout::ePresentSrcKHR;


	vk::AttachmentReference attachmentReference {};
	attachmentReference.attachment = 0;
	attachmentReference.layout	   = vk::ImageLayout::eColorAttachmentOptimal;


	vk::SubpassDescription subpassDescription {};
	subpassDescription.pipelineBindPoint	= vk::PipelineBindPoint::eGraphics;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments	= &attachmentReference;


	vk::RenderPassCreateInfo renderPassCreateInfo {};
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments	 = &attachmentDescription;
	renderPassCreateInfo.subpassCount	 = 1;
	renderPassCreateInfo.pSubpasses		 = &subpassDescription;

	RETURN_IF_VK_ERROR(vkDevice.createRenderPass(&renderPassCreateInfo, nullptr, &vkRenderPass),
					   "Failed to create render pass");


	// create framebuffer

	vkSwapchainInfo.framebuffers.resize(swapchainImageCount);
	for (uint i = 0; i < vkSwapchainInfo.framebuffers.size(); i++) {
		vk::FramebufferCreateInfo framebufferCreateInfo {};
		framebufferCreateInfo.renderPass	  = vkRenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments	  = &vkSwapchainInfo.imageViews[i];
		framebufferCreateInfo.width			  = vkSwapchainInfo.extent.width;
		framebufferCreateInfo.height		  = vkSwapchainInfo.extent.height;
		framebufferCreateInfo.layers		  = 1;

		RETURN_IF_VK_ERROR(
			vkDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &vkSwapchainInfo.framebuffers[i]),
			"Failed to create framebuffer");
	}


	// create command pool

	vk::CommandPoolCreateInfo commandPoolCreateInfo {};
	commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
	commandPoolCreateInfo.flags			   = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	RETURN_IF_VK_ERROR(vkDevice.createCommandPool(&commandPoolCreateInfo, nullptr, &vkCommandPool),
					   "Failed to create command pool");


	// allocate command buffers

	vkCommandBuffers.resize(swapchainImageCount);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
	commandBufferAllocateInfo.commandPool		 = vkCommandPool;
	commandBufferAllocateInfo.level				 = vk::CommandBufferLevel::ePrimary;
	commandBufferAllocateInfo.commandBufferCount = vkCommandBuffers.size();

	RETURN_IF_VK_ERROR(vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, vkCommandBuffers.data()),
					   "Failed to allocate command buffers");


	// create rendering semaphores

	vk::SemaphoreCreateInfo semaphoreCreateInfo {};

	RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &vkImageAvailableSemaphore),
					   "Failed to create rendering semaphore");

	RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &vkRenderingFinishedSemaphore),
					   "Failed to create rendering semaphore");


	return 0;
}


int RenderingSystem::present() {
	uint32_t imageIndex = 0;
	RETURN_IF_VK_ERROR(vkDevice.acquireNextImageKHR(
						   vkSwapchainInfo.swapchain, UINT64_MAX, vkImageAvailableSemaphore, nullptr, &imageIndex),
					   "Failed to aquire next image to present");

	vk::SubmitInfo submitInfo {};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores	  = &vkImageAvailableSemaphore;

	vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.pWaitDstStageMask			  = &pipelineStageFlags;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &vkCommandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores	= &vkRenderingFinishedSemaphore;

	RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, nullptr), "Failed to submit graphics command buffer");


	vk::PresentInfoKHR presentInfo {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores	   = &vkRenderingFinishedSemaphore;
	presentInfo.swapchainCount	   = 1;
	presentInfo.pSwapchains		   = &vkSwapchainInfo.swapchain;
	presentInfo.pImageIndices	   = &imageIndex;

	RETURN_IF_VK_ERROR(vkPresentQueue.presentKHR(&presentInfo), "Failed to present image");

	// FIXME: more optimal way
	RETURN_IF_VK_ERROR(vkPresentQueue.waitIdle(), "Failed to wait for presenting to finish");

	return 0;
}


bool RenderingSystem::isPhysicalDeviceSupported(vk::PhysicalDevice physicalDevice) const {
	auto queueFamiliesSupport = getQueueFamilies(physicalDevice).isComplete();
	bool extensionsSupport	  = checkDeviceExtensionsSupport(physicalDevice);

	auto swapChainSupportInfo = getSwapchainSupportInfo(physicalDevice);
	bool swapChainSupport	  = !swapChainSupportInfo.formats.empty() && !swapChainSupportInfo.presentModes.empty();

	return queueFamiliesSupport && extensionsSupport && swapChainSupport;


	return false;
}

RenderingSystem::QueueFamilyIndices RenderingSystem::getQueueFamilies(vk::PhysicalDevice physicalDevice) const {
	uint32_t queueFamilyCount = 0;
	physicalDevice.getQueueFamilyProperties(&queueFamilyCount, nullptr);

	std::vector<vk::QueueFamilyProperties> queueFamilies(queueFamilyCount);
	physicalDevice.getQueueFamilyProperties(&queueFamilyCount, queueFamilies.data());

	RenderingSystem::QueueFamilyIndices queueFamilyIndices;
	for (uint i = 0; i < queueFamilyCount; i++) {
		if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			queueFamilyIndices.graphicsFamily = i;
		}

		vk::Bool32 presentSupport;
		physicalDevice.getSurfaceSupportKHR(i, vkSurface, &presentSupport);
		if (presentSupport) {
			queueFamilyIndices.presentFamily = i;
		}
	}

	return queueFamilyIndices;
}

bool RenderingSystem::checkDeviceExtensionsSupport(vk::PhysicalDevice physicalDevice) const {
	uint32_t extensionCount = 0;
	RETURN_IF_VK_ERROR(physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, nullptr),
					   "Failed to enumerate device extension properties");

	std::vector<vk::ExtensionProperties> availableExtensions(extensionCount);
	RETURN_IF_VK_ERROR(
		physicalDevice.enumerateDeviceExtensionProperties(nullptr, &extensionCount, availableExtensions.data()),
		"Failed to enumerate device extension properties");

	for (auto requiredDeviceExtensionName : requiredDeviceExtensionNames) {
		auto found = false;
		for (auto extension : availableExtensions) {
			if (!strcmp(extension.extensionName, requiredDeviceExtensionName)) {
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}

	return true;
}

RenderingSystem::SwapchainSupportInfo
RenderingSystem::getSwapchainSupportInfo(vk::PhysicalDevice physicalDevice) const {
	SwapchainSupportInfo swapchainSupportInfo;

	physicalDevice.getSurfaceCapabilitiesKHR(vkSurface, &swapchainSupportInfo.capabilities);

	uint32_t formatCount = 0;
	physicalDevice.getSurfaceFormatsKHR(vkSurface, &formatCount, nullptr);
	swapchainSupportInfo.formats.resize(formatCount);
	physicalDevice.getSurfaceFormatsKHR(vkSurface, &formatCount, swapchainSupportInfo.formats.data());

	uint32_t presentModeCount = 0;
	physicalDevice.getSurfacePresentModesKHR(vkSurface, &presentModeCount, nullptr);
	swapchainSupportInfo.presentModes.resize(presentModeCount);
	physicalDevice.getSurfacePresentModesKHR(vkSurface, &presentModeCount, swapchainSupportInfo.presentModes.data());

	return swapchainSupportInfo;
}


void RenderingSystem::addRequiredInstanceExtensionNames(const std::vector<const char*>& extensionNames) {
	requiredInstanceExtensionNames.insert(
		requiredInstanceExtensionNames.end(), extensionNames.begin(), extensionNames.end());
}

void RenderingSystem::addRequiredDeviceExtensionNames(const std::vector<const char*>& extensionNames) {
	requiredDeviceExtensionNames.insert(
		requiredDeviceExtensionNames.end(), extensionNames.begin(), extensionNames.end());
}
} // namespace Engine::Systems