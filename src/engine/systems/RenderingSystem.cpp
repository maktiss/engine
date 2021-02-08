#include "RenderingSystem.hpp"

#include "engine/renderers/graphics/DepthNormalRenderer.hpp"
#include "engine/renderers/graphics/ForwardRenderer.hpp"


namespace Engine::Systems {
void RenderingSystem::RenderGraph::addInputConnection(uint srcNodeIndex, uint srcOutputIndex, uint dstNodeIndex,
													  uint dstInputIndex, bool nextFrame) {
	nodes[srcNodeIndex].inputReferenceSets[srcOutputIndex].insert({ dstNodeIndex, dstInputIndex, nextFrame });

	if (nodes[dstNodeIndex].backwardInputReferences[dstInputIndex].rendererIndex != -1) {
		spdlog::warn("Attempt to connect additional output ({}, {}) to an input slot ({}, {}), using last "
					 "specified connection",
					 srcNodeIndex, srcOutputIndex, dstNodeIndex, dstInputIndex);
	}
	nodes[dstNodeIndex].backwardInputReferences[dstInputIndex] = { srcNodeIndex, srcOutputIndex, nextFrame };
}

void RenderingSystem::RenderGraph::addOutputConnection(uint srcNodeIndex, uint srcOutputIndex, uint dstNodeIndex,
													   uint dstOutputIndex, bool nextFrame) {
	if (nodes[srcNodeIndex].outputReferences[srcOutputIndex].rendererIndex != -1) {
		spdlog::warn("Attempt to connect an output ({}, {}) to additional output slot ({}, {}), using last "
					 "specified connection",
					 srcNodeIndex, srcOutputIndex, dstNodeIndex, dstOutputIndex);
	}
	nodes[srcNodeIndex].outputReferences[srcOutputIndex] = { dstNodeIndex, dstOutputIndex, nextFrame };

	if (nodes[dstNodeIndex].backwardOutputReferences[dstOutputIndex].rendererIndex != -1) {
		spdlog::warn("Attempt to connect additional output ({}, {}) to an output slot ({}, {}), using last "
					 "specified connection",
					 srcNodeIndex, srcOutputIndex, dstNodeIndex, dstOutputIndex);
	}
	nodes[dstNodeIndex].backwardOutputReferences[dstOutputIndex] = { srcNodeIndex, srcOutputIndex, nextFrame };
}


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

	if (Engine::Managers::ShaderManager::importShaderSources<Engine::Graphics::Shaders::SimpleShader>(
			std::array<std::string, 6> { "assets/shaders/solid_color_shader.vsh", "", "", "",
										 "assets/shaders/solid_color_shader.fsh", "" })) {
		return 1;
	}


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


	Engine::Managers::MaterialManager::setVkDevice(vkDevice);
	Engine::Managers::MaterialManager::setVulkanMemoryAllocator(vmaAllocator);
	Engine::Managers::MaterialManager::init();

	// auto materialHandle = Engine::Managers::MaterialManager::createObject(0);
	// materialHandle.apply([](auto& material) {
	// 	material.color = glm::vec3(0.5f, 0.3f, 0.8f);
	// });
	// materialHandle.update();


	auto depthNormalRenderer = std::make_shared<Engine::Renderers::Graphics::DepthNormalRenderer>();
	depthNormalRenderer->setVkDevice(vkDevice);
	depthNormalRenderer->setOutputSize({ 1920, 1080 });
	depthNormalRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto forwardRenderer = std::make_shared<Engine::Renderers::Graphics::ForwardRenderer>();
	forwardRenderer->setVkDevice(vkDevice);
	forwardRenderer->setOutputSize({ 1920, 1080 });
	forwardRenderer->setVulkanMemoryAllocator(vmaAllocator);


	renderers.push_back(depthNormalRenderer);
	renderers.push_back(forwardRenderer);


	renderGraph.setNodeCount(renderers.size());
	for (uint i = 0; i < renderers.size(); i++) {
		renderGraph.setInputCount(i, renderers[i]->getInputCount());
		renderGraph.setOutputCount(i, renderers[i]->getOutputCount());
	}

	// DepthNormalRenderer depth output -> ForwardRenderer depth output
	// TODO: better readability
	renderGraph.addOutputConnection(0, 0, 1, 1);

	// ForwardRenderer color output -> swapchain image
	finalOutputReference.rendererIndex	 = 1;
	finalOutputReference.attachmentIndex = 0;


	// Lower value == higher priority
	std::vector<uint> executionPrioriries(renderers.size(), 0);

	for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
		const auto& renderer		= renderers[rendererIndex];
		const auto& renderGraphNode = renderGraph.nodes[rendererIndex];

		auto& rendererPriority = executionPrioriries[rendererIndex];

		for (uint outputIndex = 0; outputIndex < renderer->getOutputCount(); outputIndex++) {
			const auto& inputReferenceSet = renderGraphNode.inputReferenceSets[outputIndex];
			const auto& outputReference	  = renderGraphNode.outputReferences[outputIndex];

			// Deal with output first, so it will have lower priority over inputs
			if (outputReference.rendererIndex != -1) {
				if (outputReference.attachmentIndex == -1) {
					spdlog::error("Failed to compile render graph: output attachment index is not specified");
					return 1;
				}

				auto& referencedPriority = executionPrioriries[outputReference.rendererIndex];

				if (outputReference.nextFrame) {
					spdlog::error("Failed to compile render graph: output cannot be used for writing next frame");
					return 1;
				}

				if (referencedPriority <= rendererPriority) {
					referencedPriority = rendererPriority + 1;
				}

				// Shift lower priorities
				for (uint i = 0; i < executionPrioriries.size(); i++) {
					if (outputReference.rendererIndex != i) {
						if (referencedPriority <= executionPrioriries[i]) {
							executionPrioriries[i]++;
						}
					}
				}
			}

			for (const auto& inputReference : inputReferenceSet) {
				if (inputReference.rendererIndex != -1 && inputReference.attachmentIndex == -1) {
					spdlog::error("Failed to compile render graph: input attachment index is not specified");
					return 1;
				}

				auto& referencedPriority = executionPrioriries[inputReference.rendererIndex];

				if (inputReference.nextFrame) {
					// FIXME: check output dependencies
					if (rendererPriority <= referencedPriority) {
						rendererPriority = referencedPriority + 1;
					}
				} else {
					if (referencedPriority <= rendererPriority) {
						referencedPriority = rendererPriority + 1;
					}
				}

				// Shift lower priorities
				for (uint i = 0; i < executionPrioriries.size(); i++) {
					if (inputReference.rendererIndex != i) {
						if (referencedPriority <= executionPrioriries[i]) {
							executionPrioriries[i]++;
						}
					}
				}
			}
		}
	}


	// Convert priorities to ordered sequence of renderer indices

	rendererExecutionOrder.resize(renderers.size());

	std::string logMessage = "Compiled renderer execution order: ";

	uint nextPriority = 0;
	for (uint orderIndex = 0; orderIndex < rendererExecutionOrder.size(); orderIndex++) {
		auto iter = std::lower_bound(executionPrioriries.begin(), executionPrioriries.end(), nextPriority);
		assert(iter != executionPrioriries.end());

		rendererExecutionOrder[orderIndex] = iter - executionPrioriries.begin();

		nextPriority = *iter + 1;

		logMessage += std::to_string(rendererExecutionOrder[orderIndex]);
		logMessage += " ";
	}
	spdlog::warn(logMessage);


	// Iterate over render graph, set initial layouts if they are needed and create semaphores

	// FIXME: setFramesInFlightCount()
	syncObjects.resize(framesInFlightCount);
	for (auto& syncObjectsForFrame : syncObjects) {
		syncObjectsForFrame.signalSemaphores.resize(renderers.size(), {});
		syncObjectsForFrame.waitSemaphores.resize(renderers.size(), {});
	}

	for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
		const auto& renderer		= renderers[rendererIndex];
		const auto& renderGraphNode = renderGraph.nodes[rendererIndex];

		auto inputInitialLayouts = renderer->getInputInitialLayouts();
		auto outputInitialLayouts = renderer->getOutputInitialLayouts();


		for (uint inputIndex = 0; inputIndex < renderer->getInputCount(); inputIndex++) {
			if (renderGraphNode.backwardInputReferences[inputIndex].rendererIndex != -1) {
				renderer->setInputInitialLayout(inputIndex, inputInitialLayouts[inputIndex]);
			}
		}

		for (uint outputIndex = 0; outputIndex < renderer->getOutputCount(); outputIndex++) {
			if (renderGraphNode.backwardOutputReferences[outputIndex].rendererIndex != -1) {
				renderer->setOutputInitialLayout(outputIndex, outputInitialLayouts[outputIndex]);
			}

			vk::SemaphoreCreateInfo semaphoreCreateInfo {};
			vk::Semaphore semaphore {};

			for (const auto& inputReference : renderGraphNode.inputReferenceSets[outputIndex]) {
				RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
							"Failed to create rendering semaphore");

				for (uint frameInFlight = 0; frameInFlight < framesInFlightCount; frameInFlight++) {
					syncObjects[frameInFlight].signalSemaphores[rendererIndex].push_back(semaphore);
					syncObjects[frameInFlight].waitSemaphores[inputReference.rendererIndex].push_back(semaphore);
				}

				// TODO: nextFrame dependency
			}

			const auto& outputReference = renderGraphNode.outputReferences[outputIndex];
			if (outputReference.rendererIndex != -1) {
				RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
							"Failed to create rendering semaphore");

				for (uint frameInFlight = 0; frameInFlight < framesInFlightCount; frameInFlight++) {
					syncObjects[frameInFlight].signalSemaphores[rendererIndex].push_back(semaphore);
					syncObjects[frameInFlight].waitSemaphores[outputReference.rendererIndex].push_back(semaphore);
				}
			}
		}
	}


	// Iterate over render graph again to link initial and final render pass layouts

	for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
		const auto& renderer		= renderers[rendererIndex];
		const auto& renderGraphNode = renderGraph.nodes[rendererIndex];

		auto& rendererPriority = executionPrioriries[rendererIndex];

		for (uint outputIndex = 0; outputIndex < renderer->getOutputCount(); outputIndex++) {
			const auto& inputReferenceSet = renderGraphNode.inputReferenceSets[outputIndex];
			const auto& outputReference	  = renderGraphNode.outputReferences[outputIndex];

			std::vector<RenderGraph::NodeReference> sortedInputReferences {};
			if (!inputReferenceSet.empty()) {
				// Sort input references by execution priority
				for (uint executionIndex = 0; executionIndex < rendererExecutionOrder.size(); executionIndex++) {
					const auto& iter = inputReferenceSet.find({ rendererExecutionOrder[executionIndex], {}, {} });
					if (iter != inputReferenceSet.end()) {
						sortedInputReferences.push_back(*iter);
					}
				}

				renderer->setOutputFinalLayout(
					outputIndex, renderers[sortedInputReferences[0].rendererIndex]
									 ->getInputInitialLayouts()[sortedInputReferences[0].attachmentIndex]);

				for (uint inputReferenceIndex = 0; inputReferenceIndex < sortedInputReferences.size() - 1;
					 inputReferenceIndex++) {
					const auto& inputReference	   = sortedInputReferences[inputReferenceIndex];
					const auto& nextInputReference = sortedInputReferences[inputReferenceIndex + 1];

					renderers[inputReference.rendererIndex]->setInputFinalLayout(
						inputReference.attachmentIndex,
						renderers[nextInputReference.rendererIndex]
							->getInputInitialLayouts()[nextInputReference.attachmentIndex]);
				}
			}

			if (outputReference.rendererIndex != -1) {
				if (sortedInputReferences.empty()) {
					renderer->setOutputFinalLayout(outputIndex,
												   renderers[outputReference.rendererIndex]
													   ->getOutputInitialLayouts()[outputReference.attachmentIndex]);
				} else {
					const auto& lastOutputReference = sortedInputReferences[sortedInputReferences.size() - 1];
					renderers[lastOutputReference.rendererIndex]->setInputFinalLayout(
						lastOutputReference.attachmentIndex,
						renderers[outputReference.rendererIndex]
							->getOutputInitialLayouts()[outputReference.attachmentIndex]);
				}
			}
		}
	}

	// Set final layout for output to blit result from
	renderers[finalOutputReference.rendererIndex]->setOutputFinalLayout(finalOutputReference.attachmentIndex,
																		vk::ImageLayout::eTransferSrcOptimal);

	// Iterate over render graph again to create and link inputs/outputs

	for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
		const auto& renderer = renderers[rendererIndex];
		const auto& renderGraphNode = renderGraph.nodes[rendererIndex];
		const auto& outputDescriptions = renderer->getOutputDescriptions();
		
		for (uint outputIndex = 0; outputIndex < outputDescriptions.size(); outputIndex++) {
			if (renderGraphNode.backwardOutputReferences[outputIndex].rendererIndex == -1) {
				auto textureHandle = Engine::Managers::TextureManager::createObject(0);

				bool isFinal = false;
				if (finalOutputReference.rendererIndex == rendererIndex && finalOutputReference.attachmentIndex == outputIndex) {
					isFinal = true;
				}

				textureHandle.apply([&outputDescriptions, outputIndex, isFinal](auto& textureHandle) {
					textureHandle.format = outputDescriptions[outputIndex].format;
					textureHandle.usage	 = outputDescriptions[outputIndex].usage;

					if (textureHandle.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
						textureHandle.imageAspect = vk::ImageAspectFlagBits::eDepth;
					}

					// FIXME: output size
					textureHandle.size = vk::Extent3D(1920, 1080, 1);

					if (isFinal) {
						textureHandle.usage |= vk::ImageUsageFlagBits::eTransferSrc;
					}
				});
				textureHandle.update();

				renderer->setOutput(outputIndex, textureHandle);

				for (auto& inputReference : renderGraphNode.inputReferenceSets[outputIndex]) {
					renderers[inputReference.rendererIndex]->setInput(inputReference.attachmentIndex, textureHandle);
				}

				// Iterate over chain of outputs
				auto outputReference = renderGraphNode.outputReferences[outputIndex];
				while (outputReference.rendererIndex != -1) {
					renderers[outputReference.rendererIndex]->setOutput(outputReference.attachmentIndex, textureHandle);
					outputReference = renderGraph.nodes[outputReference.rendererIndex].outputReferences[outputReference.attachmentIndex];
				}
			}
		}

		renderer->init();
	}

	finalTextureHandle = renderers[finalOutputReference.rendererIndex]->getOutput(finalOutputReference.attachmentIndex);

	// for (auto& renderer : renderers) {
	// 	const auto& outputDescriptions = renderer->getOutputDescriptions();
	// 	for (uint outputIndex = 0; outputIndex < outputDescriptions.size(); outputIndex++) {
	// 		auto textureHandle = Engine::Managers::TextureManager::createObject(0);

	// 		textureHandle.apply([&outputDescriptions, outputIndex](auto& textureHandle) {
	// 			textureHandle.format = outputDescriptions[outputIndex].format;
	// 			textureHandle.usage	 = outputDescriptions[outputIndex].usage;

	// 			if (textureHandle.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
	// 				textureHandle.imageAspect = vk::ImageAspectFlagBits::eDepth;
	// 			}

	// 			// FIXME: output size
	// 			textureHandle.size = vk::Extent3D(1920, 1080, 1);

	// 			// FIXME: for final texture only
	// 			textureHandle.usage |= vk::ImageUsageFlagBits::eTransferSrc;
	// 		});
	// 		textureHandle.update();

	// 		renderer->setOutput(outputIndex, textureHandle);

	// 		// FIXME base on render graph
	// 		// renderer->setOutputInitialLayout(outputIndex, vk::ImageLayout::eUndefined);
	// 		// renderer->setOutputFinalLayout(outputIndex, vk::ImageLayout::eTransferSrcOptimal);
	// 	}

	// 	// FIXME
	// 	finalTextureHandle = renderer->getOutput(0);

	// 	renderer->init();
	// }


	// Allocate command buffers for every renderer

	vkCommandBuffers.resize(framesInFlightCount * renderers.size() * (1 + threadCount));

	for (uint frameIndex = 0; frameIndex < framesInFlightCount; frameIndex++) {
		for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
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

	vkCommandBufferFences.resize(framesInFlightCount * renderers.size());
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

	// uint32_t fenceCount = vkCommandPools.size() / (framesInFlightCount * 2 * (1 + threadCount));
	RETURN_IF_VK_ERROR(vkDevice.waitForFences(renderers.size(),
											  &vkCommandBufferFences[currentFrameInFlight * renderers.size()], true,
											  UINT64_MAX),
					   "Failed to wait for command buffer fences");
	RETURN_IF_VK_ERROR(
		vkDevice.resetFences(renderers.size(), &vkCommandBufferFences[currentFrameInFlight * renderers.size()]),
		"Failed to reset command buffer fences");


	// Wait for image blit command buffers to finish

	RETURN_IF_VK_ERROR(
		vkDevice.waitForFences(1, &vkImageBlitCommandBufferFences[currentFrameInFlight], true, UINT64_MAX),
		"Failed to wait for image blit command buffer fences");
	RETURN_IF_VK_ERROR(vkDevice.resetFences(1, &vkImageBlitCommandBufferFences[currentFrameInFlight]),
					   "Failed to reset image blit command buffer fences");


	// Reset command buffers

	vkDevice.resetCommandPool(vkCommandPools[commandPoolIndex]);


	for (uint orderIndex = 0; orderIndex < rendererExecutionOrder.size(); orderIndex++) {
		auto rendererIndex = rendererExecutionOrder[orderIndex];
		auto& renderer = renderers[rendererIndex];

		auto commandBufferIndex = getCommandBufferIndex(currentFrameInFlight, rendererIndex, 0);

		auto& commandBuffer		 = vkCommandBuffers[commandBufferIndex];
		auto& commandBufferFence = vkCommandBufferFences[currentFrameInFlight * renderers.size() + rendererIndex];


		renderer->render(commandBuffer, dt);


		vk::SubmitInfo submitInfo {};
		submitInfo.waitSemaphoreCount = syncObjects[currentFrameInFlight].waitSemaphores[rendererIndex].size();
		submitInfo.pWaitSemaphores	  = syncObjects[currentFrameInFlight].waitSemaphores[rendererIndex].data();

		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eBottomOfPipe;
		submitInfo.pWaitDstStageMask			  = &pipelineStageFlags;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers	  = &commandBuffer;

		submitInfo.signalSemaphoreCount = syncObjects[currentFrameInFlight].signalSemaphores[rendererIndex].size();
		submitInfo.pSignalSemaphores	= syncObjects[currentFrameInFlight].signalSemaphores[rendererIndex].data();

		RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, commandBufferFence),
						   "Failed to submit command buffer");
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
	RETURN_IF_VK_ERROR(vkDevice.acquireNextImageKHR(vkSwapchainInfo.swapchain, UINT64_MAX, imageAvailableSemaphore,
													nullptr, &imageIndex),
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

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, 0,
								  nullptr, 0, nullptr, 1, &imageMemoryBarrier0);

	commandBuffer.blitImage(srcImage, vk::ImageLayout::eTransferSrcOptimal, dstImage,
							vk::ImageLayout::eTransferDstOptimal, 1, &imageBlitRegion, vk::Filter::eLinear);

	commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTopOfPipe, {}, 0,
								  nullptr, 0, nullptr, 1, &imageMemoryBarrier1);

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
	requiredInstanceExtensionNames.insert(requiredInstanceExtensionNames.end(), extensionNames.begin(),
										  extensionNames.end());
}

void RenderingSystem::addRequiredDeviceExtensionNames(const std::vector<const char*>& extensionNames) {
	requiredDeviceExtensionNames.insert(requiredDeviceExtensionNames.end(), extensionNames.begin(),
										extensionNames.end());
}
} // namespace Engine::Systems