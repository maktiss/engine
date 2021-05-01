#include "RenderingSystem.hpp"

#include "engine/renderers/Renderers.hpp"

#include "engine/utils/CPUTimer.hpp"


namespace Engine {
void RenderingSystem::RenderGraph::addInputConnection(std::string srcName, uint srcOutputIndex, std::string dstName,
													  uint dstInputIndex, bool nextFrame) {
	nodes[srcName].inputReferenceSets[srcOutputIndex].insert({ dstName, dstInputIndex, nextFrame });

	if (!nodes[dstName].backwardInputReferences[dstInputIndex].rendererName.empty()) {
		spdlog::warn("Attempt to connect additional output ({}, {}) to an input slot ({}, {}), using last "
					 "specified connection",
					 srcName, srcOutputIndex, dstName, dstInputIndex);
	}
	nodes[dstName].backwardInputReferences[dstInputIndex] = { srcName, srcOutputIndex, nextFrame };
}

void RenderingSystem::RenderGraph::addOutputConnection(std::string srcName, uint srcOutputIndex, std::string dstName,
													   uint dstOutputIndex, bool nextFrame) {
	if (!nodes[srcName].outputReferences[srcOutputIndex].rendererName.empty()) {
		spdlog::warn("Attempt to connect an output ({}, {}) to additional output slot ({}, {}), using last "
					 "specified connection",
					 srcName, srcOutputIndex, dstName, dstOutputIndex);
	}
	nodes[srcName].outputReferences[srcOutputIndex] = { dstName, dstOutputIndex, nextFrame };

	if (!nodes[dstName].backwardOutputReferences[dstOutputIndex].rendererName.empty()) {
		spdlog::warn("Attempt to connect additional output ({}, {}) to an output slot ({}, {}), using last "
					 "specified connection",
					 srcName, srcOutputIndex, dstName, dstOutputIndex);
	}
	nodes[dstName].backwardOutputReferences[dstOutputIndex] = { srcName, srcOutputIndex, nextFrame };
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

	GraphicsShaderManager::setVkDevice(vkDevice);
	GraphicsShaderManager::init();

	if (GraphicsShaderManager::importShaderSources<SimpleShader>(std::array<std::string, 6> {
			"assets/shaders/material_generic.vsh", "", "", "", "assets/shaders/material_generic.fsh", "" })) {
		return 1;
	}

	if (GraphicsShaderManager::importShaderSources<SkyboxShader>(
			std::array<std::string, 6> { "assets/shaders/skybox.vsh", "", "", "", "assets/shaders/skybox.fsh", "" })) {
		return 1;
	}

	if (GraphicsShaderManager::importShaderSources<SkymapShader>(
			std::array<std::string, 6> { "assets/shaders/skymap.vsh", "", "", "", "assets/shaders/skymap.fsh", "" })) {
		return 1;
	}

	if (GraphicsShaderManager::importShaderSources<PostFxShader>(
			std::array<std::string, 6> { "assets/shaders/postfx.vsh", "", "", "", "assets/shaders/postfx.fsh", "" })) {
		return 1;
	}

	if (GraphicsShaderManager::importShaderSources<ReflectionShader>(std::array<std::string, 6> {
			"assets/shaders/reflection.vsh", "", "", "", "assets/shaders/reflection.fsh", "" })) {
		return 1;
	}


	TextureManager::setVkDevice(vkDevice);
	TextureManager::setVkCommandPool(vkCommandPools[0]);
	TextureManager::setVkTransferQueue(vkGraphicsQueue);
	TextureManager::setVulkanMemoryAllocator(vmaAllocator);
	TextureManager::init();

	// auto textureHandle = TextureManager::createObject(0);
	// auto swapchainInfo = vkSwapchainInfo;
	// textureHandle.apply([swapchainInfo](auto& texture){
	// 	texture.size = vk::Extent3D(swapchainInfo.extent.width, swapchainInfo.extent.height, 1);
	// 	texture.format = swapchainInfo.imageFormat;
	// 	texture.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	// });
	// textureHandle.update();


	MeshManager::setVkDevice(vkDevice);
	MeshManager::setVkCommandPool(vkCommandPools[0]);
	MeshManager::setVkTransferQueue(vkGraphicsQueue);
	MeshManager::setVulkanMemoryAllocator(vmaAllocator);
	MeshManager::init();


	MaterialManager::setVkDevice(vkDevice);
	MaterialManager::setVulkanMemoryAllocator(vmaAllocator);
	MaterialManager::init();

	// auto materialHandle = MaterialManager::createObject(0);
	// materialHandle.apply([](auto& material) {
	// 	material.color = glm::vec3(0.5f, 0.3f, 0.8f);
	// });
	// materialHandle.update();


	auto depthNormalRenderer = std::make_shared<DepthNormalRenderer>();
	depthNormalRenderer->setVkDevice(vkDevice);
	depthNormalRenderer->setOutputSize({ 1920, 1080 });
	depthNormalRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto forwardRenderer = std::make_shared<ForwardRenderer>();
	forwardRenderer->setVkDevice(vkDevice);
	forwardRenderer->setOutputSize({ 1920, 1080 });
	forwardRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto shadowMapRenderer = std::make_shared<ShadowMapRenderer>();
	shadowMapRenderer->setVkDevice(vkDevice);
	shadowMapRenderer->setOutputSize({ 1920, 1080 });
	shadowMapRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto skyboxRenderer = std::make_shared<SkyboxRenderer>();
	skyboxRenderer->setVkDevice(vkDevice);
	skyboxRenderer->setOutputSize({ 1920, 1080 });
	skyboxRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto skymapRenderer = std::make_shared<SkymapRenderer>();
	skymapRenderer->setVkDevice(vkDevice);
	skymapRenderer->setOutputSize({ 1024, 1024 });
	skymapRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto imGuiRenderer = std::make_shared<ImGuiRenderer>();
	imGuiRenderer->setVkDevice(vkDevice);
	imGuiRenderer->setOutputSize({ 1920, 1080 });
	imGuiRenderer->setVulkanMemoryAllocator(vmaAllocator);
	imGuiRenderer->setVkInstance(vkInstance);
	imGuiRenderer->setVkPhysicalDevice(getActivePhysicalDevice());
	imGuiRenderer->setVkGraphicsQueue(vkGraphicsQueue);

	auto postFxRenderer = std::make_shared<PostFxRenderer>();
	postFxRenderer->setVkDevice(vkDevice);
	postFxRenderer->setOutputSize({ 1920, 1080 });
	postFxRenderer->setVulkanMemoryAllocator(vmaAllocator);

	auto reflectionRenderer = std::make_shared<ReflectionRenderer>();
	reflectionRenderer->setVkDevice(vkDevice);
	reflectionRenderer->setOutputSize({ 1920, 1080 });
	reflectionRenderer->setVulkanMemoryAllocator(vmaAllocator);


	renderers["DepthNormalRenderer"] = depthNormalRenderer;
	renderers["ForwardRenderer"]	 = forwardRenderer;
	renderers["ShadowMapRenderer"]	 = shadowMapRenderer;
	renderers["SkyboxRenderer"]		 = skyboxRenderer;
	renderers["SkymapRenderer"]		 = skymapRenderer;
	renderers["ImGuiRenderer"]		 = imGuiRenderer;
	renderers["PostFxRenderer"]		 = postFxRenderer;
	renderers["ReflectionRenderer"]	 = reflectionRenderer;


	for (const auto& [name, renderer] : renderers) {
		renderGraph.setInputCount(name, renderer->getInputCount());
		renderGraph.setOutputCount(name, renderer->getOutputCount());
	}


	renderGraph.addInputConnection("SkymapRenderer", 0, "SkyboxRenderer", 0);

	renderGraph.addInputConnection("DepthNormalRenderer", 1, "ReflectionRenderer", 0);
	renderGraph.addInputConnection("DepthNormalRenderer", 0, "ReflectionRenderer", 1);
	renderGraph.addInputConnection("SkymapRenderer", 0, "ReflectionRenderer", 2);

	renderGraph.addInputConnection("ShadowMapRenderer", 0, "ForwardRenderer", 0);
	renderGraph.addInputConnection("DepthNormalRenderer", 0, "ForwardRenderer", 1);
	renderGraph.addInputConnection("ReflectionRenderer", 0, "ForwardRenderer", 2);

	renderGraph.addOutputConnection("SkyboxRenderer", 0, "ForwardRenderer", 0);
	renderGraph.addOutputConnection("DepthNormalRenderer", 1, "ForwardRenderer", 1);

	renderGraph.addInputConnection("ForwardRenderer", 0, "PostFxRenderer", 0);

	renderGraph.addOutputConnection("PostFxRenderer", 0, "ImGuiRenderer", 0);


	finalOutputReference = { "ImGuiRenderer", 0 };


	spdlog::info("Compiling render graph...");

	// Lower value == higher priority
	std::unordered_map<std::string, uint> executionPrioriries {};

	for (const auto& [rendererName, renderer] : renderers) {
		const auto& renderGraphNode = renderGraph.nodes[rendererName];

		auto& rendererPriority = executionPrioriries[rendererName];

		for (const auto backwardInputReference : renderGraphNode.backwardInputReferences) {
			if (!backwardInputReference.rendererName.empty()) {
				auto& referencedPriority = executionPrioriries[backwardInputReference.rendererName];

				if (rendererPriority <= referencedPriority) {
					rendererPriority = referencedPriority + 1;
				}

				// Shift lower priorities
				for (auto& [rendererName, priority] : executionPrioriries) {
					if (backwardInputReference.rendererName != rendererName) {
						if (referencedPriority <= priority) {
							priority++;
						}
					}
				}
			}
		}

		for (const auto backwardOutputReference : renderGraphNode.backwardOutputReferences) {
			if (!backwardOutputReference.rendererName.empty()) {
				auto& referencedPriority = executionPrioriries[backwardOutputReference.rendererName];

				if (rendererPriority <= referencedPriority) {
					rendererPriority = referencedPriority + 1;
				}

				// Shift lower priorities
				for (auto& [rendererName, priority] : executionPrioriries) {
					if (backwardOutputReference.rendererName != rendererName) {
						if (referencedPriority <= priority) {
							priority++;
						}
					}
				}
			}
		}

		for (uint outputIndex = 0; outputIndex < renderer->getOutputCount(); outputIndex++) {
			const auto& inputReferenceSet = renderGraphNode.inputReferenceSets[outputIndex];
			const auto& outputReference	  = renderGraphNode.outputReferences[outputIndex];

			// Deal with output first, so it will have lower priority over inputs
			if (!outputReference.rendererName.empty()) {
				if (outputReference.attachmentIndex == -1) {
					spdlog::error("Failed to compile render graph: output attachment index is not specified");
					return 1;
				}

				auto& referencedPriority = executionPrioriries[outputReference.rendererName];

				if (outputReference.nextFrame) {
					spdlog::error("Failed to compile render graph: output cannot be used for writing next frame");
					return 1;
				}

				if (referencedPriority <= rendererPriority) {
					referencedPriority = rendererPriority + 1;
				}

				// Shift lower priorities
				for (auto& [rendererName, priority] : executionPrioriries) {
					if (outputReference.rendererName != rendererName) {
						if (referencedPriority <= priority) {
							priority++;
						}
					}
				}
			}

			for (const auto& inputReference : inputReferenceSet) {
				if (!inputReference.rendererName.empty() && inputReference.attachmentIndex == -1) {
					spdlog::error("Failed to compile render graph: input attachment index is not specified");
					return 1;
				}

				auto& referencedPriority = executionPrioriries[inputReference.rendererName];

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
				for (auto& [rendererName, priority] : executionPrioriries) {
					if (inputReference.rendererName != rendererName) {
						if (referencedPriority <= priority) {
							priority++;
						}
					}
				}
			}
		}
	}


	// Deal with duplicate priorities

	for (const auto& [firstRendererName, firstPriority] : executionPrioriries) {
		for (auto& [secondRendererName, secondPriority] : executionPrioriries) {
			if (firstRendererName != secondRendererName && firstPriority <= secondPriority) {
				secondPriority++;
			}
		}
	}


	// Convert priorities to ordered sequence of renderer names

	rendererExecutionOrder.resize(renderers.size());

	std::string logMessage = "Compiled renderer execution order:";

	uint nextPriority = 0;
	for (uint orderIndex = 0; orderIndex < rendererExecutionOrder.size(); orderIndex++) {
		auto iter = executionPrioriries.end();
		while (iter == executionPrioriries.end()) {
			for (iter = executionPrioriries.begin(); iter != executionPrioriries.end(); iter++) {
				if (iter->second == nextPriority) {
					break;
				}
			}
			nextPriority++;
		}

		rendererExecutionOrder[orderIndex] = iter->first;
		rendererIndexMap[iter->first]	   = orderIndex;

		logMessage += "\n\t";
		logMessage += std::to_string(orderIndex + 1) + ". ";
		logMessage += rendererExecutionOrder[orderIndex];
	}
	spdlog::warn(logMessage);


	// Iterate over render graph, set initial layouts if they are needed and create semaphores

	// Temporary vectors for easier indexing
	std::vector<std::vector<std::vector<vk::Semaphore>>> rendererWaitSemaphores(
		framesInFlightCount, std::vector<std::vector<vk::Semaphore>>(renderers.size()));
	std::vector<std::vector<std::vector<vk::Semaphore>>> rendererSignalSemaphores(
		framesInFlightCount, std::vector<std::vector<vk::Semaphore>>(renderers.size()));

	for (const auto& [rendererName, renderer] : renderers) {
		const auto rendererIndex = getRendererIndex(rendererName);

		const auto& renderGraphNode = renderGraph.nodes[rendererName];

		auto inputInitialLayouts  = renderer->getInputInitialLayouts();
		auto outputInitialLayouts = renderer->getOutputInitialLayouts();


		for (uint inputIndex = 0; inputIndex < renderer->getInputCount(); inputIndex++) {
			if (!renderGraphNode.backwardInputReferences[inputIndex].rendererName.empty()) {
				renderer->setInputInitialLayout(inputIndex, inputInitialLayouts[inputIndex]);
			}
		}

		for (uint outputIndex = 0; outputIndex < renderer->getOutputCount(); outputIndex++) {
			if (!renderGraphNode.backwardOutputReferences[outputIndex].rendererName.empty()) {
				renderer->setOutputInitialLayout(outputIndex, outputInitialLayouts[outputIndex]);
			}

			vk::SemaphoreCreateInfo semaphoreCreateInfo {};
			vk::Semaphore semaphore {};

			for (const auto& inputReference : renderGraphNode.inputReferenceSets[outputIndex]) {
				for (uint frameInFlight = 0; frameInFlight < framesInFlightCount; frameInFlight++) {
					RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
									   "Failed to create rendering semaphore");

					rendererWaitSemaphores[frameInFlight][getRendererIndex(inputReference.rendererName)].push_back(
						semaphore);
					rendererSignalSemaphores[frameInFlight][rendererIndex].push_back(semaphore);
				}

				// TODO: nextFrame dependency
			}

			const auto& outputReference = renderGraphNode.outputReferences[outputIndex];
			if (!outputReference.rendererName.empty()) {
				for (uint frameInFlight = 0; frameInFlight < framesInFlightCount; frameInFlight++) {
					RETURN_IF_VK_ERROR(vkDevice.createSemaphore(&semaphoreCreateInfo, nullptr, &semaphore),
									   "Failed to create rendering semaphore");

					rendererWaitSemaphores[frameInFlight][getRendererIndex(outputReference.rendererName)].push_back(
						semaphore);
					rendererSignalSemaphores[frameInFlight][rendererIndex].push_back(semaphore);
				}
			}
		}
	}


	// Index renderer semaphores

	vkRendererWaitSemaphoresViews.resize(framesInFlightCount * renderers.size());
	vkRendererSignalSemaphoresViews.resize(framesInFlightCount * renderers.size());

	uint rendererWaitSemaphoresOffset	= 0;
	uint rendererSignalSemaphoresOffset = 0;

	for (uint frameInFlight = 0; frameInFlight < framesInFlightCount; frameInFlight++) {
		for (uint rendererIndex = 0; rendererIndex < renderers.size(); rendererIndex++) {
			auto& waitSemaphoresView = vkRendererWaitSemaphoresViews[frameInFlight * renderers.size() + rendererIndex];
			auto& signalSemaphoresView =
				vkRendererSignalSemaphoresViews[frameInFlight * renderers.size() + rendererIndex];

			waitSemaphoresView.first  = rendererWaitSemaphoresOffset;
			waitSemaphoresView.second = rendererWaitSemaphores[frameInFlight][rendererIndex].size();

			rendererWaitSemaphoresOffset += waitSemaphoresView.second;

			signalSemaphoresView.first	= rendererSignalSemaphoresOffset;
			signalSemaphoresView.second = rendererSignalSemaphores[frameInFlight][rendererIndex].size();

			rendererSignalSemaphoresOffset += signalSemaphoresView.second;

			vkRendererWaitSemaphores.insert(vkRendererWaitSemaphores.end(),
											rendererWaitSemaphores[frameInFlight][rendererIndex].begin(),
											rendererWaitSemaphores[frameInFlight][rendererIndex].end());
			vkRendererSignalSemaphores.insert(vkRendererSignalSemaphores.end(),
											  rendererSignalSemaphores[frameInFlight][rendererIndex].begin(),
											  rendererSignalSemaphores[frameInFlight][rendererIndex].end());
		}
	}


	// Iterate over render graph again to link initial and final render pass layouts

	for (const auto& [rendererName, renderer] : renderers) {
		const auto& renderGraphNode = renderGraph.nodes[rendererName];

		auto& rendererPriority = executionPrioriries[rendererName];

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
					outputIndex, renderers[sortedInputReferences[0].rendererName]
									 ->getInputInitialLayouts()[sortedInputReferences[0].attachmentIndex]);

				for (uint inputReferenceIndex = 0; inputReferenceIndex < sortedInputReferences.size() - 1;
					 inputReferenceIndex++) {
					const auto& inputReference	   = sortedInputReferences[inputReferenceIndex];
					const auto& nextInputReference = sortedInputReferences[inputReferenceIndex + 1];

					renderers[inputReference.rendererName]->setInputFinalLayout(
						inputReference.attachmentIndex,
						renderers[nextInputReference.rendererName]
							->getInputInitialLayouts()[nextInputReference.attachmentIndex]);
				}
			}

			if (!outputReference.rendererName.empty()) {
				if (sortedInputReferences.empty()) {
					renderer->setOutputFinalLayout(outputIndex,
												   renderers[outputReference.rendererName]
													   ->getOutputInitialLayouts()[outputReference.attachmentIndex]);
				} else {
					const auto& lastOutputReference = sortedInputReferences[sortedInputReferences.size() - 1];
					renderers[lastOutputReference.rendererName]->setInputFinalLayout(
						lastOutputReference.attachmentIndex,
						renderers[outputReference.rendererName]
							->getOutputInitialLayouts()[outputReference.attachmentIndex]);
				}
			}
		}
	}

	// Set final layout for output to blit result from
	renderers[finalOutputReference.rendererName]->setOutputFinalLayout(finalOutputReference.attachmentIndex,
																	   vk::ImageLayout::eTransferSrcOptimal);


	// Iterate over render graph again to create and link inputs/outputs

	for (const auto& [rendererName, renderer] : renderers) {
		const auto& renderGraphNode	   = renderGraph.nodes[rendererName];
		const auto& outputDescriptions = renderer->getOutputDescriptions();

		const auto outputSize = renderer->getOutputSize();

		const auto layerCount = renderer->getLayerCount() * renderer->getMultiviewLayerCount();

		for (uint outputIndex = 0; outputIndex < outputDescriptions.size(); outputIndex++) {
			if (renderGraphNode.backwardOutputReferences[outputIndex].rendererName.empty()) {
				auto textureHandle = TextureManager::createObject(0);

				bool isFinal = false;
				if (finalOutputReference.rendererName == rendererName &&
					finalOutputReference.attachmentIndex == outputIndex) {
					isFinal = true;
				}

				vk::ImageCreateFlags imageFlags {};
				for (auto& inputReference : renderGraphNode.inputReferenceSets[outputIndex]) {
					for (const auto& inputDescription :
						 renderers[inputReference.rendererName]->getInputDescriptions()) {
						imageFlags |= inputDescription.flags;
					}
				}

				textureHandle.apply([&](auto& texture) {
					texture.format = outputDescriptions[outputIndex].format;
					texture.usage  = outputDescriptions[outputIndex].usage;
					texture.flags  = outputDescriptions[outputIndex].flags | imageFlags;

					texture.layerCount = layerCount;

					if (texture.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
						texture.imageAspect = vk::ImageAspectFlagBits::eDepth;
					}

					texture.size = vk::Extent3D(outputSize.width, outputSize.height, 1);

					if (isFinal) {
						texture.usage |= vk::ImageUsageFlagBits::eTransferSrc;
					}
				});
				textureHandle.update();

				renderer->setOutput(outputIndex, textureHandle);

				for (auto& inputReference : renderGraphNode.inputReferenceSets[outputIndex]) {
					renderers[inputReference.rendererName]->setInput(inputReference.attachmentIndex, textureHandle);
				}

				// Iterate over chain of outputs
				auto outputReference = renderGraphNode.outputReferences[outputIndex];
				while (!outputReference.rendererName.empty()) {
					const auto& inputReferenceSet = renderGraph.nodes[outputReference.rendererName]
														.inputReferenceSets[outputReference.attachmentIndex];
					for (auto& inputReference : inputReferenceSet) {
						renderers[inputReference.rendererName]->setInput(inputReference.attachmentIndex, textureHandle);
					}

					renderers[outputReference.rendererName]->setOutput(outputReference.attachmentIndex, textureHandle);

					outputReference = renderGraph.nodes[outputReference.rendererName]
										  .outputReferences[outputReference.attachmentIndex];
				}
			}
		}
	}

	for (const auto& [rendererName, renderer] : renderers) {
		renderer->init();
	}

	finalTextureHandle = renderers[finalOutputReference.rendererName]->getOutput(finalOutputReference.attachmentIndex);

	// for (auto& renderer : renderers) {
	// 	const auto& outputDescriptions = renderer->getOutputDescriptions();
	// 	for (uint outputIndex = 0; outputIndex < outputDescriptions.size(); outputIndex++) {
	// 		auto textureHandle = TextureManager::createObject(0);

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


	// Allocate command buffers for each renderer and their layers

	vkPrimaryCommandBuffers.resize(0);
	vkSecondaryCommandBuffers.resize(0);

	vkPrimaryCommandBuffersViews.resize(framesInFlightCount * renderers.size());
	vkSecondaryCommandBuffersViews.resize(framesInFlightCount * renderers.size());

	for (uint frameIndex = 0; frameIndex < framesInFlightCount; frameIndex++) {
		for (const auto& [rendererName, renderer] : renderers) {
			auto rendererIndex = getRendererIndex(rendererName);

			auto& primaryCommandBuffersView	  = getPrimaryCommandBuffersView(frameIndex, rendererIndex);
			auto& secondaryCommandBuffersView = getSecondaryCommandBuffersView(frameIndex, rendererIndex);

			primaryCommandBuffersView.first	  = vkPrimaryCommandBuffers.size();
			secondaryCommandBuffersView.first = vkSecondaryCommandBuffers.size();

			const auto renderLayerCount = renderers[rendererName]->getLayerCount();

			primaryCommandBuffersView.second   = renderLayerCount;
			secondaryCommandBuffersView.second = renderLayerCount * threadCount;

			for (uint rendererLayer = 0; rendererLayer < renderLayerCount; rendererLayer++) {
				for (uint threadIndex = 0; threadIndex < (1 + threadCount); threadIndex++) {
					uint commandPoolIndex = getCommandPoolIndex(frameIndex, threadIndex);

					vk::CommandBufferAllocateInfo commandBufferAllocateInfo {};
					commandBufferAllocateInfo.commandPool		 = vkCommandPools[commandPoolIndex];
					commandBufferAllocateInfo.commandBufferCount = 1;

					if (threadIndex == 0) {
						commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
					} else {
						commandBufferAllocateInfo.level = vk::CommandBufferLevel::eSecondary;
					}

					vk::CommandBuffer commandBuffer {};
					RETURN_IF_VK_ERROR(vkDevice.allocateCommandBuffers(&commandBufferAllocateInfo, &commandBuffer),
									   "Failed to allocate command buffer");

					if (threadIndex == 0) {
						vkPrimaryCommandBuffers.push_back(commandBuffer);
					} else {
						vkSecondaryCommandBuffers.push_back(commandBuffer);
					}
				}
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

	vkRendererFences.resize(framesInFlightCount * renderers.size());
	for (auto& fence : vkRendererFences) {
		RETURN_IF_VK_ERROR(vkDevice.createFence(&fenceCreateInfo, nullptr, &fence),
						   "Failed to create command buffer fence");
	}

	vkImageBlitCommandBufferFences.resize(framesInFlightCount);
	for (auto& fence : vkImageBlitCommandBufferFences) {
		RETURN_IF_VK_ERROR(vkDevice.createFence(&fenceCreateInfo, nullptr, &fence),
						   "Failed to create image blit command buffer fence");
	}


	// Create query pools

	vkTimestampQueryPools.resize(framesInFlightCount * renderers.size());

	vk::QueryPoolCreateInfo queryPoolCreateInfo {};
	queryPoolCreateInfo.queryType = vk::QueryType::eTimestamp;

	auto& debugState = GlobalStateManager::getWritable<DebugState>();

	uint maxQueries = 0;
	for (const auto& [rendererName, renderer] : renderers) {
		auto rendererIndex = getRendererIndex(rendererName);
		auto queryCount	   = renderer->getLayerCount() * renderer->getMultiviewLayerCount() * 2;

		maxQueries = std::max(maxQueries, queryCount);

		debugState.rendererExecutionTimes[rendererName].resize(queryCount / 2);

		for (uint frameIndex = 0; frameIndex < framesInFlightCount; frameIndex++) {
			queryPoolCreateInfo.queryCount = queryCount;

			RETURN_IF_VK_ERROR(vkDevice.createQueryPool(&queryPoolCreateInfo, nullptr,
														&getTimestampQueryPool(frameIndex, rendererIndex)),
							   "Failed to create timestamp query pool");
		}
	}

	timestampQueriesBuffer.resize(maxQueries);


	// FIXME not hardcoded index
	auto& executionTimes = debugState.executionTimeArrays[3];

	for (const auto& rendererName : rendererExecutionOrder) {
		auto& renderer = renderers[rendererName];

		const auto layerCount		   = renderer->getLayerCount();
		const auto multiviewLayerCount = renderer->getMultiviewLayerCount();

		DebugState::ExecutionTime executionTime {};
		executionTime.level = 1;
		executionTime.name	= rendererName;
		executionTimes.push_back(executionTime);

		for (uint layerIndex = 0; layerIndex < layerCount; layerIndex++) {
			executionTime.level = 2;
			executionTime.name	= "Layer " + std::to_string(layerIndex);

			if (layerCount > 1 || multiviewLayerCount > 1) {
				executionTimes.push_back(executionTime);
			}

			for (uint multiviewLayerIndex = 0; multiviewLayerIndex < multiviewLayerCount; multiviewLayerIndex++) {
				executionTime.level = 3;
				executionTime.name	= "MV Layer " + std::to_string(multiviewLayerIndex);

				if (multiviewLayerCount > 1) {
					executionTimes.push_back(executionTime);
				}
			}
		}
	}
	executionTimes.push_back({ 1, "SwapchainPresent" });

	return 0;
}

int RenderingSystem::run(double dt) {
	currentFrameInFlight = (currentFrameInFlight + 1) % framesInFlightCount;


	// Wait for rendering command buffers to finish

	// uint32_t fenceCount = vkCommandPools.size() / (framesInFlightCount * 2 * (1 + threadCount));
	RETURN_IF_VK_ERROR(vkDevice.waitForFences(renderers.size(),
											  &vkRendererFences[currentFrameInFlight * renderers.size()], true,
											  UINT64_MAX),
					   "Failed to wait for command buffer fences");
	RETURN_IF_VK_ERROR(
		vkDevice.resetFences(renderers.size(), &vkRendererFences[currentFrameInFlight * renderers.size()]),
		"Failed to reset command buffer fences");


	// Wait for image blit command buffers to finish

	RETURN_IF_VK_ERROR(
		vkDevice.waitForFences(1, &vkImageBlitCommandBufferFences[currentFrameInFlight], true, UINT64_MAX),
		"Failed to wait for image blit command buffer fences");
	RETURN_IF_VK_ERROR(vkDevice.resetFences(1, &vkImageBlitCommandBufferFences[currentFrameInFlight]),
					   "Failed to reset image blit command buffer fences");


	// Query timestamps

	auto& debugState = GlobalStateManager::getWritable<DebugState>();

	// FIXME not hardcoded index
	auto& executionTimes = debugState.executionTimeArrays[3];

	uint executionTimeIndex = 1;

	for (const auto& rendererName : rendererExecutionOrder) {
		auto& renderer	   = renderers[rendererName];
		auto rendererIndex = getRendererIndex(rendererName);

		const auto& queryPool = getTimestampQueryPool(currentFrameInFlight, rendererIndex);

		const auto layerCount		   = renderer->getLayerCount();
		const auto multiviewLayerCount = renderer->getMultiviewLayerCount();

		const auto queryCount = layerCount * multiviewLayerCount * 2;

		vkDevice.getQueryPoolResults(queryPool, 0, queryCount, sizeof(uint64_t) * queryCount,
									 timestampQueriesBuffer.data(), sizeof(uint64_t), vk::QueryResultFlagBits::e64);

		auto& times = debugState.rendererExecutionTimes[rendererName];
		for (uint i = 0; i < times.size(); i++) {
			// FIXME multiply by timeperiod
			times[i] =
				(timestampQueriesBuffer[i * 2 + multiviewLayerCount] - timestampQueriesBuffer[i * 2]) / 1'000'000.0f;
		}


		float totalRendererTime {};

		for (uint layerIndex = 0; layerIndex < layerCount; layerIndex++) {
			float totalLayerTime {};

			if (layerCount > 1 || multiviewLayerCount > 1) {
				executionTimeIndex++;
			}

			for (uint multiviewLayerIndex = 0; multiviewLayerIndex < multiviewLayerCount; multiviewLayerIndex++) {
				// FIXME multiply by timeperiod
				float multiviewLayerTime =
					(timestampQueriesBuffer[layerIndex * 2 * multiviewLayerCount + multiviewLayerCount] -
					 timestampQueriesBuffer[layerIndex * 2 * multiviewLayerCount + multiviewLayerIndex]) /
					1'000'000.0f;

				if (multiviewLayerCount > 1) {
					executionTimeIndex++;
					executionTimes[executionTimeIndex].gpuTime = multiviewLayerTime;
				}

				totalLayerTime += multiviewLayerTime;
			}


			if (layerCount > 1 || multiviewLayerCount > 1) {
				if (multiviewLayerCount > 1) {
					executionTimes[executionTimeIndex - multiviewLayerCount].gpuTime = totalLayerTime;
				} else {
					executionTimes[executionTimeIndex].gpuTime = totalLayerTime;
				}
			}

			totalRendererTime += totalLayerTime;
		}

		// Calculate offset to get total execution time index
		uint offset = 0;
		if (layerCount > 1) {
			offset = layerCount;
		}
		if (multiviewLayerCount > 1) {
			offset = layerCount + layerCount * multiviewLayerCount;
		}

		executionTimes[executionTimeIndex - offset].cpuTime = cpuTimings[rendererName];
		executionTimes[executionTimeIndex - offset].gpuTime = totalRendererTime;

		executionTimeIndex++;
	}
	executionTimes[executionTimeIndex].cpuTime = cpuTimings["SwapchainPresent"];


	// Reset command buffers

	auto commandPoolIndex = getCommandPoolIndex(currentFrameInFlight, 0);

	for (uint threadIndex = 0; threadIndex < (threadCount + 1); threadIndex++) {
		vkDevice.resetCommandPool(vkCommandPools[commandPoolIndex + threadIndex]);
	}


	for (const auto& rendererName : rendererExecutionOrder) {
		CPUTimer cpuTimer {};
		cpuTimer.start();

		auto& renderer	   = renderers[rendererName];
		auto rendererIndex = getRendererIndex(rendererName);

		const auto& primaryCommandBuffersView = getPrimaryCommandBuffersView(currentFrameInFlight, rendererIndex);
		const auto* pPrimaryCommandBuffers	  = &vkPrimaryCommandBuffers[primaryCommandBuffersView.first];
		const auto primaryCommandBuffersCount = primaryCommandBuffersView.second;

		const auto& secondarycommandBuffersView = getSecondaryCommandBuffersView(currentFrameInFlight, rendererIndex);
		const auto* pSecondaryCommandBuffers	= &vkSecondaryCommandBuffers[secondarycommandBuffersView.first];
		const auto secondaryCommandBuffersCount = secondarycommandBuffersView.second;

		const auto& timestampQueryPool = getTimestampQueryPool(currentFrameInFlight, rendererIndex);


		renderer->render(pPrimaryCommandBuffers, pSecondaryCommandBuffers, timestampQueryPool, dt);


		const auto& rendererWaitSemaphoresViews = getRendererWaitSemaphoresView(currentFrameInFlight, rendererIndex);
		const auto* pRendererWaitSemaphores		= &vkRendererWaitSemaphores[rendererWaitSemaphoresViews.first];
		const auto rendererWaitSemaphoresCount	= rendererWaitSemaphoresViews.second;

		const auto& rendererSignalSemaphoresViews =
			getRendererSignalSemaphoresView(currentFrameInFlight, rendererIndex);
		const auto* pRendererSignalSemaphores	 = &vkRendererSignalSemaphores[rendererSignalSemaphoresViews.first];
		const auto rendererSignalSemaphoresCount = rendererSignalSemaphoresViews.second;

		vk::SubmitInfo submitInfo {};
		submitInfo.pWaitSemaphores	  = pRendererWaitSemaphores;
		submitInfo.waitSemaphoreCount = rendererWaitSemaphoresCount;

		// FIXME: proper wait semaphores dst stage masks
		vk::PipelineStageFlags pipelineStageFlags[] = {
			vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
			vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe,
		};
		submitInfo.pWaitDstStageMask = pipelineStageFlags;

		submitInfo.commandBufferCount = primaryCommandBuffersCount;
		submitInfo.pCommandBuffers	  = pPrimaryCommandBuffers;

		submitInfo.pSignalSemaphores	= pRendererSignalSemaphores;
		submitInfo.signalSemaphoreCount = rendererSignalSemaphoresCount;

		auto& rendererFence = vkRendererFences[currentFrameInFlight * renderers.size() + rendererIndex];

		RETURN_IF_VK_ERROR(vkGraphicsQueue.submit(1, &submitInfo, rendererFence), "Failed to submit command buffer");

		cpuTimings[rendererName] = cpuTimer.stop();
	}


	if (present()) {
		return 1;
	}

	return 0;
}


int RenderingSystem::present() {
	CPUTimer cpuTimer {};
	cpuTimer.start();

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

	auto& srcImage = TextureManager::getTextureInfo(finalTextureHandle).image;
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

	cpuTimings["SwapchainPresent"] = cpuTimer.stop();

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
	physicalDeviceFeatures.samplerAnisotropy = true;

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

	auto surfaceFormat = swapchainSupportInfo.formats[0];
	for (auto format : swapchainSupportInfo.formats) {
		if (format.format == vk::Format::eB8G8R8A8Srgb) {
			surfaceFormat = format;
		}
	}

	auto presentMode = vk::PresentModeKHR::eImmediate;
	switch (vSync) {
	case 1:
		presentMode = vk::PresentModeKHR::eFifo;
		break;
	}

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
} // namespace Engine