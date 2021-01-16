#include "RenderingSystem.hpp"


namespace Engine::Systems {
int RenderingSystem::init() {
	spdlog::info("Initializing RenderingSystem...");


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

	if (initVulkanMemoryAllocator()) {
		return 1;
	}

	if (createSwapchain()) {
		return 1;
	}


	// FIXME testing

	Engine::Managers::ShaderManager::setVkDevice(vkDevice);
	Engine::Managers::ShaderManager::init();

	Engine::Managers::ShaderManager::importShaderSources<Engine::Graphics::Shaders::SimpleShader>(
		std::array<std::string, 6> {
			"assets/shaders/solid_color_shader.vsh", "", "", "", "assets/shaders/solid_color_shader.fsh", "" });


	Engine::Managers::TextureManager::setVkDevice(vkDevice);
	Engine::Managers::TextureManager::setVkCommandPool(vkCommandPools[0]);
	Engine::Managers::TextureManager::setVkTransferQueue(vkGraphicsQueue);
	Engine::Managers::TextureManager::setVulkanMemoryAllocator(vmaAllocator);
	Engine::Managers::TextureManager::init();

	// auto textureHandle = Engine::Managers::TextureManager::createObject(0);
	// auto swapchainInfo = vkSwapchainInfo;
	// textureHandle.apply([swapchainInfo](auto& texture){
	// 	texture.size = vk::Extent3D(swapchainInfo.extent.width, swapchainInfo.extent.height, 1);
	// 	texture.format = swapchainInfo.imageFormat;
	// 	texture.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	// });
	// textureHandle.update();


	Engine::Managers::MeshManager::setVkDevice(vkDevice);
	Engine::Managers::MeshManager::setVkCommandPool(vkCommandPools[0]);
	Engine::Managers::MeshManager::setVkTransferQueue(vkGraphicsQueue);
	Engine::Managers::MeshManager::setVulkanMemoryAllocator(vmaAllocator);
	Engine::Managers::MeshManager::init();

	meshHandle = Engine::Managers::MeshManager::createObject(0);
	meshHandle.update();


	Engine::Managers::MaterialManager::setVkDevice(vkDevice);
	Engine::Managers::MaterialManager::setVulkanMemoryAllocator(vmaAllocator);
	Engine::Managers::MaterialManager::init();

	// auto materialHandle = Engine::Managers::MaterialManager::createObject(0);
	// materialHandle.apply([](auto& material) {
	// 	material.color = glm::vec3(0.5f, 0.3f, 0.8f);
	// });
	// materialHandle.update();


	forwardRenderer.setVkDevice(vkDevice);
	forwardRenderer.setOutputSize({ 1920, 1080 });
	forwardRenderer.setVulkanMemoryAllocator(vmaAllocator);
	forwardRenderer.init();
	// forwardRenderer.allocateCommandBuffers(vkCommandPools);

	if (createRenderPasses()) {
		return 1;
	}
	if (createFramebuffers()) {
		return 1;
	}

	// FIXME for each renderer
	if (generateGraphicsPipelines(&forwardRenderer)) {
		return 1;
	}


	// Allocate command buffers for every renderer

	// FIXME: renderers array size
	uint renderersCount = 1;
	vkCommandBuffers.resize(framesInFlightCount * renderersCount * (1 + threadCount));

	for (uint frameIndex = 0; frameIndex < framesInFlightCount; frameIndex++) {
		for (uint rendererIndex = 0; rendererIndex < renderersCount; rendererIndex++) {
			for (uint threadIndex = 0; threadIndex < 1 + threadCount; threadIndex++) {
				uint commandPoolIndex	= getCommandPoolIndex(frameIndex, threadIndex);
				uint commandBufferIndex = getCommandBufferIndex(frameIndex, rendererIndex, threadIndex);

				vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
				commandBufferAllocateInfo.commandPool		 = vkCommandPools[commandPoolIndex];
				commandBufferAllocateInfo.commandBufferCount = 1;

				commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;


				RETURN_IF_VK_ERROR(
					vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &vkCommandBuffers[commandBufferIndex]),
					"Failed to allocate command buffer");
			}
		}
	}


	// Allocate command buffers for image blit

	vkImageBlitCommandBuffers.resize(framesInFlightCount);
	for (uint frameIndex = 0; frameIndex < framesInFlightCount; frameIndex++) {
		uint commandPoolIndex = getCommandPoolIndex(frameIndex, 0);

		vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
		commandBufferAllocateInfo.commandPool		 = vkCommandPools[commandPoolIndex];
		commandBufferAllocateInfo.commandBufferCount = 1;

		commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;


		RETURN_IF_VK_ERROR(
			vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &vkImageBlitCommandBuffers[frameIndex]),
			"Failed to allocate command buffer");
	}


	// Create semaphores

	vk::SemaphoreCreateInfo semaphoreCreateInfo {};

	vkImageAvailableSemaphores.resize(framesInFlightCount);
	vkImageBlitFinishedSemaphores.resize(framesInFlightCount);

	for (auto& semaphore : vkImageAvailableSemaphores) {
		RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
						   "Failed to create rendering semaphore");
	}
	for (auto& semaphore : vkImageBlitFinishedSemaphores) {
		RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
						   "Failed to create rendering semaphore");
	}


	// Create command buffer fences

	vk::FenceCreateInfo fenceCreateInfo {};
	fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	vkCommandBufferFences.resize(vkCommandBuffers.size() / (1 + threadCount));
	for (auto& fence : vkCommandBufferFences) {
		RETURN_IF_VK_ERROR(vkDevice.createFence(&fenceCreateInfo, nullptr, &fence),
						   "Failed to create command buffer fence");
	}

	vkImageBlitCommandBufferFences.resize(framesInFlightCount);
	for (auto& fence : vkImageBlitCommandBufferFences) {
		RETURN_IF_VK_ERROR(vkDevice.createFence(&fenceCreateInfo, nullptr, &fence),
						   "Failed to create image blit command buffer fence");
	}

	return 0;
}

int RenderingSystem::run(double dt) {
	currentFrameInFlight  = (currentFrameInFlight + 1) % framesInFlightCount;
	auto commandPoolIndex = getCommandPoolIndex(currentFrameInFlight, 0);


	// Wait for rendering command buffers to finish

	uint32_t fenceCount = vkCommandPools.size() / (framesInFlightCount * (1 + threadCount));
	RETURN_IF_VK_ERROR(
		vkDevice.waitForFences(fenceCount, &vkCommandBufferFences[currentFrameInFlight * fenceCount], true, UINT64_MAX),
		"Failed to wait for command buffer fences");
	RETURN_IF_VK_ERROR(vkDevice.resetFences(fenceCount, &vkCommandBufferFences[currentFrameInFlight * fenceCount]),
					   "Failed to reset command buffer fences");


	// Wait for image blit command buffers to finish

	RETURN_IF_VK_ERROR(
		vkDevice.waitForFences(1, &vkImageBlitCommandBufferFences[currentFrameInFlight], true, UINT64_MAX),
		"Failed to wait for image blit command buffer fences");
	RETURN_IF_VK_ERROR(vkDevice.resetFences(fenceCount, &vkImageBlitCommandBufferFences[currentFrameInFlight]),
					   "Failed to reset image blit command buffer fences");


	// Reset command buffers

	vkDevice.resetCommandPool(vkCommandPools[commandPoolIndex]);


	// FIXME indexed renderers
	for (uint rendererIndex = 0; rendererIndex < 1; rendererIndex++) {
		auto& renderer = forwardRenderer;

		auto commandBufferIndex = getCommandBufferIndex(currentFrameInFlight, rendererIndex, 0);

		auto& commandBuffer		 = vkCommandBuffers[commandBufferIndex];
		auto& commandBufferFence = vkCommandBufferFences[commandBufferIndex / (1 + threadCount)];


		vk::CommandBufferBeginInfo commandBufferBeginInfo {};

		vk::RenderPassBeginInfo renderPassBeginInfo {};
		renderPassBeginInfo.renderPass		  = vkRenderPasses[rendererIndex];
		renderPassBeginInfo.framebuffer		  = vkFramebuffers[rendererIndex];
		renderPassBeginInfo.renderArea.extent = vkSwapchainInfo.extent;
		renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);

		auto& clearValues					= renderer.getVkClearValues();
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues	= clearValues.data();


		// Begin command buffer

		RETURN_IF_VK_ERROR(commandBuffer.begin(&commandBufferBeginInfo), "Failed to record command buffer");

		commandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

		renderer.recordCommandBuffer(dt, commandBuffer);

		commandBuffer.endRenderPass();
		RETURN_IF_VK_ERROR(commandBuffer.end(), "Failed to record command buffer");


		vk::SubmitInfo submitInfo {};
		// submitInfo.waitSemaphoreCount = 1;
		// submitInfo.pWaitSemaphores	  = &vkImageAvailableSemaphore;

		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		submitInfo.pWaitDstStageMask			  = &pipelineStageFlags;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers	  = &commandBuffer;

		// submitInfo.signalSemaphoreCount = 1;
		// submitInfo.pSignalSemaphores	= &vkRenderingFinishedSemaphore;

		RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, commandBufferFence),
						   "Failed to submit graphics command buffer");
	}


	if (present()) {
		return 1;
	}

	return 0;
}


int RenderingSystem::present() {
	auto& imageAvailableSemaphore	 = vkImageAvailableSemaphores[currentFrameInFlight];
	auto& imageBlitFinishedSemaphore = vkImageBlitFinishedSemaphores[currentFrameInFlight];

	uint32_t imageIndex = 0;
	RETURN_IF_VK_ERROR(vkDevice.acquireNextImageKHR(
						   vkSwapchainInfo.swapchain, UINT64_MAX, imageAvailableSemaphore, nullptr, &imageIndex),
					   "Failed to aquire next image to present");


	vk::ImageBlit imageBlitRegion {};

	imageBlitRegion.srcSubresource.aspectMask	  = vk::ImageAspectFlagBits::eColor;
	imageBlitRegion.srcSubresource.mipLevel		  = 0;
	imageBlitRegion.srcSubresource.baseArrayLayer = 0;
	imageBlitRegion.srcSubresource.layerCount	  = 1;
	imageBlitRegion.srcOffsets[0]				  = vk::Offset3D { 0, 0, 0 };
	imageBlitRegion.srcOffsets[1]				  = vk::Offset3D { 1920, 1080, 1 };

	imageBlitRegion.dstSubresource.aspectMask	  = vk::ImageAspectFlagBits::eColor;
	imageBlitRegion.dstSubresource.mipLevel		  = 0;
	imageBlitRegion.dstSubresource.baseArrayLayer = 0;
	imageBlitRegion.dstSubresource.layerCount	  = 1;
	imageBlitRegion.dstOffsets[0]				  = vk::Offset3D { 0, 0, 0 };
	imageBlitRegion.dstOffsets[1]				  = vk::Offset3D { 1920, 1080, 1 };


	auto& commandBuffer = vkImageBlitCommandBuffers[currentFrameInFlight];

	vk::CommandBufferBeginInfo commandBufferBeginInfo {};

	auto& srcImage = Engine::Managers::TextureManager::getTextureInfo(finalTextureHandle).image;
	auto& dstImage = vkSwapchainInfo.images[imageIndex];


	// TODO set image layout helper function
	vk::ImageMemoryBarrier imageMemoryBarrier0 {};
	imageMemoryBarrier0.oldLayout					  = vk::ImageLayout::ePresentSrcKHR;
	imageMemoryBarrier0.newLayout					  = vk::ImageLayout::eTransferDstOptimal;
	imageMemoryBarrier0.srcAccessMask				  = {};
	imageMemoryBarrier0.dstAccessMask				  = vk::AccessFlagBits::eTransferWrite;
	imageMemoryBarrier0.image						  = dstImage;
	imageMemoryBarrier0.subresourceRange.aspectMask	  = vk::ImageAspectFlagBits::eColor;
	imageMemoryBarrier0.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier0.subresourceRange.levelCount	  = 1;
	imageMemoryBarrier0.subresourceRange.layerCount	  = 1;

	vk::ImageMemoryBarrier imageMemoryBarrier1 {};
	imageMemoryBarrier1.oldLayout					  = vk::ImageLayout::eTransferDstOptimal;
	imageMemoryBarrier1.newLayout					  = vk::ImageLayout::ePresentSrcKHR;
	imageMemoryBarrier1.srcAccessMask				  = vk::AccessFlagBits::eTransferWrite;
	imageMemoryBarrier1.dstAccessMask				  = vk::AccessFlagBits::eMemoryRead;
	imageMemoryBarrier1.image						  = dstImage;
	imageMemoryBarrier1.subresourceRange.aspectMask	  = vk::ImageAspectFlagBits::eColor;
	imageMemoryBarrier1.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier1.subresourceRange.levelCount	  = 1;
	imageMemoryBarrier1.subresourceRange.layerCount	  = 1;


	// Write image blit command buffer

	RETURN_IF_VK_ERROR(commandBuffer.begin(&commandBufferBeginInfo), "Failed to record command buffer");

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
								  vk::PipelineStageFlagBits::eTransfer,
								  {},
								  0,
								  nullptr,
								  0,
								  nullptr,
								  1,
								  &imageMemoryBarrier0);

	commandBuffer.blitImage(srcImage,
							vk::ImageLayout::eTransferSrcOptimal,
							dstImage,
							vk::ImageLayout::eTransferDstOptimal,
							1,
							&imageBlitRegion,
							vk::Filter::eLinear);

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
								  vk::PipelineStageFlagBits::eTopOfPipe,
								  {},
								  0,
								  nullptr,
								  0,
								  nullptr,
								  1,
								  &imageMemoryBarrier1);

	RETURN_IF_VK_ERROR(commandBuffer.end(), "Failed to record command buffer");


	// Submit image blit command buffer

	vk::SubmitInfo submitInfo {};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores	  = &imageAvailableSemaphore;

	// TODO: what should be here?
	vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	submitInfo.pWaitDstStageMask			  = &pipelineStageFlags;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers	  = &commandBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores	= &imageBlitFinishedSemaphore;

	RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, vkImageBlitCommandBufferFences[currentFrameInFlight]),
					   "Failed to submit graphics command buffer");


	// Present an image

	vk::PresentInfoKHR presentInfo {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores	   = &imageBlitFinishedSemaphore;
	presentInfo.swapchainCount	   = 1;
	presentInfo.pSwapchains		   = &vkSwapchainInfo.swapchain;
	presentInfo.pImageIndices	   = &imageIndex;

	RETURN_IF_VK_ERROR(vkPresentQueue.presentKHR(&presentInfo), "Failed to present image");

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


int RenderingSystem::initVulkanMemoryAllocator() {
	VmaAllocatorCreateInfo vmaAllocatorCreateInfo {};
	vmaAllocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	vmaAllocatorCreateInfo.instance			= vkInstance;
	vmaAllocatorCreateInfo.device			= VkDevice(vkDevice);
	vmaAllocatorCreateInfo.physicalDevice	= VkPhysicalDevice(getActivePhysicalDevice());

	auto result = vmaCreateAllocator(&vmaAllocatorCreateInfo, &vmaAllocator);
	if (result != VK_SUCCESS) {
		spdlog::error("Failed to initialize Vulkan Memory Allocator. Error code: {}", result);
		return 1;
	}
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
	swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;

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

	vkSwapchainInfo.extent = extent;


	// create command pools

	vk::CommandPoolCreateInfo commandPoolCreateInfo {};
	commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
	// commandPoolCreateInfo.flags			   = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	vkCommandPools.resize(framesInFlightCount * (1 + threadCount));
	for (uint i = 0; i < vkCommandPools.size(); i++) {
		RETURN_IF_VK_ERROR(vkDevice.createCommandPool(&commandPoolCreateInfo, nullptr, &vkCommandPools[i]),
						   "Failed to create command pool");
	}


	return 0;
}


int RenderingSystem::createRenderPasses() {

	// FIXME
	vkRenderPasses.resize(1);

	// TODO: for each renderer
	createRenderPass(&forwardRenderer, vkRenderPasses[0]);

	return 0;
}

int RenderingSystem::createFramebuffers() {

	// FIXME
	vkFramebuffers.resize(1);

	auto outputDescriptions = forwardRenderer.getOutputDescriptions();

	std::vector<Engine::Managers::TextureManager::Handle> textures(outputDescriptions.size());
	std::vector<vk::ImageView> imageViews(textures.size());

	for (uint i = 0; i < textures.size(); i++) {
		auto& texture = textures[i];
		texture		  = Engine::Managers::TextureManager::createObject(0);

		texture.apply([&outputDescriptions, i](auto& texture) {
			texture.format = outputDescriptions[i].format;
			texture.usage  = outputDescriptions[i].usage;

			if (texture.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
				texture.imageAspect = vk::ImageAspectFlagBits::eDepth;
			}

			// FIXME: output size
			texture.size = vk::Extent3D(1920, 1080, 1);

			// FIXME: for final texture only
			texture.usage |= vk::ImageUsageFlagBits::eTransferSrc;
		});
		texture.update();

		imageViews[i] = Engine::Managers::TextureManager::getTextureInfo(texture).imageView;
	}

	// FIXME
	finalTextureHandle = textures[0];

	vk::FramebufferCreateInfo framebufferCreateInfo {};
	framebufferCreateInfo.renderPass	  = vkRenderPasses[0];
	framebufferCreateInfo.attachmentCount = imageViews.size();
	framebufferCreateInfo.pAttachments	  = imageViews.data();

	// FIXME: output size
	framebufferCreateInfo.width	 = 1920;
	framebufferCreateInfo.height = 1080;
	framebufferCreateInfo.layers = 1;

	RETURN_IF_VK_ERROR(vkDevice.createFramebuffer(&framebufferCreateInfo, nullptr, &vkFramebuffers[0]),
					   "Failed to create framebuffer");

	return 0;
}


int RenderingSystem::createRenderPass(Engine::Renderers::RendererBase* renderer, vk::RenderPass& renderPass) {
	auto attachmentDescriptions = renderer->getVkAttachmentDescriptions();

	// FIXME: should depend on rendergraph
	for (auto& attachmentDescription : attachmentDescriptions) {
		attachmentDescription.initialLayout = vk::ImageLayout::eUndefined;
		attachmentDescription.finalLayout	= vk::ImageLayout::eTransferSrcOptimal;
	}

	const auto& attachmentReferences = renderer->getVkAttachmentReferences();

	vk::SubpassDescription subpassDescription {};
	subpassDescription.pipelineBindPoint	= renderer->getVkPipelineBindPoint();
	subpassDescription.colorAttachmentCount = attachmentReferences.size();
	subpassDescription.pColorAttachments	= attachmentReferences.data();

	if (attachmentReferences.size() > 0) {
		auto& lastAttachmentReference = attachmentReferences[attachmentReferences.size() - 1];
		// TODO: check for all depth layout variations
		if (lastAttachmentReference.layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			subpassDescription.colorAttachmentCount--;
			subpassDescription.pDepthStencilAttachment = &lastAttachmentReference;
		}
	}


	vk::RenderPassCreateInfo renderPassCreateInfo {};
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments	 = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount	 = 1;
	renderPassCreateInfo.pSubpasses		 = &subpassDescription;

	RETURN_IF_VK_ERROR(vkDevice.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass),
					   "Failed to create render pass");

	return 0;
}

int RenderingSystem::generateGraphicsPipelines(Engine::Renderers::RendererBase* renderer) {
	constexpr auto shaderTypeCount = Engine::Managers::ShaderManager::getTypeCount();
	constexpr auto meshTypeCount   = Engine::Managers::MeshManager::getTypeCount();

	auto renderPassIndex = Engine::Managers::ShaderManager::getRenderPassIndex(renderer->getRenderPassName());

	// FIXME index
	vk::RenderPass renderPass = vkRenderPasses[0];
	// createRenderPass(renderer, renderPass);

	const auto& pipelineInputAssemblyStateCreateInfo = renderer->getVkPipelineInputAssemblyStateCreateInfo();
	const auto& viewport							 = renderer->getVkViewport();
	const auto& scissor								 = renderer->getVkScissor();
	const auto& pipelineRasterizationStateCreateInfo = renderer->getVkPipelineRasterizationStateCreateInfo();
	const auto& pipelineMultisampleStateCreateInfo	 = renderer->getVkPipelineMultisampleStateCreateInfo();
	const auto& pipelineDepthStencilStateCreateInfo	 = renderer->getVkPipelineDepthStencilStateCreateInfo();
	const auto& pipelineColorBlendAttachmentState	 = renderer->getVkPipelineColorBlendAttachmentState();

	vk::PipelineViewportStateCreateInfo pipelineViewportStateCreateInfo {};
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports	  = &viewport;
	pipelineViewportStateCreateInfo.scissorCount  = 1;
	pipelineViewportStateCreateInfo.pScissors	  = &scissor;

	vk::PipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo {};
	pipelineColorBlendStateCreateInfo.logicOpEnable	  = false;
	pipelineColorBlendStateCreateInfo.attachmentCount = 1;
	pipelineColorBlendStateCreateInfo.pAttachments	  = &pipelineColorBlendAttachmentState;


	auto descriptorSetLayouts = renderer->getVkDescriptorSetLayouts();
	descriptorSetLayouts.push_back(Engine::Managers::MaterialManager::getVkDescriptorSetLayout());

	// TODO: request from renderer
	vk::PushConstantRange pushConstantRange {};
	pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eAll;
	pushConstantRange.offset	 = 0;
	pushConstantRange.size		 = 64;

	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts	= descriptorSetLayouts.data();

	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges	= &pushConstantRange;

	vk::PipelineLayout pipelineLayout {};
	RETURN_IF_VK_ERROR(vkDevice.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &pipelineLayout),
					   "Failed to create pipeline layout");

	renderer->setVkPipelineLayout(pipelineLayout);


	std::vector<vk::Pipeline> graphicsPipelines {};
	for (uint shaderTypeIndex = 0; shaderTypeIndex < shaderTypeCount; shaderTypeIndex++) {
		for (uint meshTypeIndex = 0; meshTypeIndex < meshTypeCount; meshTypeIndex++) {
			vk::PipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo {};

			auto vertexAttributeDescriptions =
				Engine::Managers::MeshManager::getVertexInputAttributeDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = vertexAttributeDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions	   = vertexAttributeDescriptions.data();

			auto vertexBindingDescriptions =
				Engine::Managers::MeshManager::getVertexInputBindingDescriptions(meshTypeIndex);
			pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = vertexBindingDescriptions.size();
			pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions	 = vertexBindingDescriptions.data();

			auto flagCount = Engine::Managers::ShaderManager::getShaderFlagCount(shaderTypeIndex);

			for (uint signature = 0; signature < std::pow(2, flagCount); signature++) {
				uint32_t shaderStageCount = 0;
				vk::PipelineShaderStageCreateInfo pipelineShaderStageCreateInfos[6];

				auto shaderInfo = Engine::Managers::ShaderManager::getShaderInfo(
					renderPassIndex, shaderTypeIndex, meshTypeIndex, signature);
				for (uint shaderStageIndex = 0; shaderStageIndex < 6; shaderStageIndex++) {
					if (shaderInfo.shaderModules[shaderStageIndex] != vk::ShaderModule()) {
						switch (shaderStageIndex) {
						case 0:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eVertex;
							break;
						case 1:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eGeometry;
							break;
						case 2:
							pipelineShaderStageCreateInfos[shaderStageCount].stage =
								vk::ShaderStageFlagBits::eTessellationControl;
							break;
						case 3:
							pipelineShaderStageCreateInfos[shaderStageCount].stage =
								vk::ShaderStageFlagBits::eTessellationEvaluation;
							break;
						case 4:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eFragment;
							break;
						case 5:
							pipelineShaderStageCreateInfos[shaderStageCount].stage = vk::ShaderStageFlagBits::eCompute;
							break;
						}
						pipelineShaderStageCreateInfos[shaderStageCount].module =
							shaderInfo.shaderModules[shaderStageIndex];
						pipelineShaderStageCreateInfos[shaderStageCount].pName = "main";
						shaderStageCount++;
					}
				}

				vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo {};
				graphicsPipelineCreateInfo.stageCount = shaderStageCount;
				graphicsPipelineCreateInfo.pStages	  = pipelineShaderStageCreateInfos;

				graphicsPipelineCreateInfo.pVertexInputState   = &pipelineVertexInputStateCreateInfo;
				graphicsPipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
				graphicsPipelineCreateInfo.pViewportState	   = &pipelineViewportStateCreateInfo;
				graphicsPipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
				graphicsPipelineCreateInfo.pMultisampleState   = &pipelineMultisampleStateCreateInfo;
				graphicsPipelineCreateInfo.pDepthStencilState  = &pipelineDepthStencilStateCreateInfo;
				graphicsPipelineCreateInfo.pColorBlendState	   = &pipelineColorBlendStateCreateInfo;
				graphicsPipelineCreateInfo.pDynamicState	   = nullptr;

				graphicsPipelineCreateInfo.layout = pipelineLayout;

				graphicsPipelineCreateInfo.renderPass = renderPass;
				graphicsPipelineCreateInfo.subpass	  = 0;

				vk::Pipeline pipeline;
				RETURN_IF_VK_ERROR(
					vkDevice.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline),
					"Failed to create graphics pipeline");

				graphicsPipelines.push_back(pipeline);
			}
		}
	}
	renderer->setGraphicsPipelines(graphicsPipelines);

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