#include <stdexcept>
#include <cstring>
#include "video.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../../lib/stb/stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../../lib/tinyobjloader/tiny_obj_loader.h"

#include "shaders/object_vert.spv"
#include "shaders/object_frag.spv"
#include "shaders/interface_vert.spv"
#include "shaders/interface_frag.spv"

// Validation layers
#ifndef VALIDATE
#define VALIDATE true
#endif

#include <iostream>

Video::Video(const std::string& name, int width, int height)
{
	ApplicationName = name;
	FOV = 45.0f;

	InitWindow(width, height);
	InitVulkan();
}

Video::~Video()
{
	CloseVulkan();
	CloseWindow();
}

void Video::InitWindow(int width, int height)
{
	bool success = glfwInit();

	if (!success) {
		throw std::runtime_error("failed to init GLFW");
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(
		width,
		height,
		ApplicationName.c_str(),
		nullptr,
		nullptr);

	if (!window) {
		glfwTerminate();
		throw std::runtime_error("failed to create window");
	}

	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	glfwSetKeyCallback(window, KeyActionCallback);
	glfwSetCursorPosCallback(window, CursorMoveCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);
}

void Video::CloseWindow()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Video::InitVulkan()
{
	deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	validate = VALIDATE;

	if (validate) {
		validationLayers = GetValidationLayers();
	}

	vertexBufferMemoryManager = nullptr;
	indexBufferMemoryManager = nullptr;
	uniformBufferMemoryManager = nullptr;
	textureImageMemoryManager = nullptr;
	allowDescriptorPoolCreation = false;
	allowUniformBufferCreation = false;
	allowVertexBufferCreation = false;
	allowIndexBufferCreation = false;
	allowDescriptorSetCreation = false;
	allowTextureImageCreation = false;
	framebufferResized = false;
	cameraCursor = false;

	cursorMoveController = nullptr;
	cursorMoveCallback = nullptr;
	mouseButtonController = nullptr;
	mouseButtonCallback = nullptr;
	scrollController = nullptr;
	scrollCallback = nullptr;

	skybox = nullptr;

	CreateInstance();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPools();
	CreateSyncObjects();
	CreateDescriptorSetLayout();
	CreateVertexBuffers();
	CreateIndexBuffers();
	CreateTextureImages();

	CreateSwapchain();
}

void Video::CloseVulkan()
{
	DestroySwapchain();

	DestroyTextureImages();
	DestroyIndexBuffers();
	DestroyVertexBuffers();
	DestroyDescriptorSetLayout();
	DestroySyncObjects();
	DestroyCommandPools();
	DestroySkybox();

	if (vertexBufferMemoryManager) {
		delete vertexBufferMemoryManager;
	}

	if (indexBufferMemoryManager) {
		delete indexBufferMemoryManager;
	}

	if (uniformBufferMemoryManager) {
		delete uniformBufferMemoryManager;
	}

	if (textureImageMemoryManager) {
		delete textureImageMemoryManager;
	}

	DestroyLogicalDevice();
	DestroySurface();
	DestroyInstance();
}

void Video::CreateSwapchain()
{
	CreateSwapchainInstance();
	CreateColorResources();
	CreateDepthResources();
	CreateImageViews();
	CreateSwapchainSyncObjects();
	CreateDescriptorPools();
	CreateUniformBuffers();
	CreateDescriptorSets();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();
	CreateCommandBuffers();
}

void Video::DestroySwapchain()
{
	DestroyCommandBuffers();
	DestroyFramebuffers();
	DestroyGraphicsPipeline();
	DestroyRenderPass();
	DestroyUniformBuffers();
	DestroyDescriptorPools();
	DestroySwapchainSyncObjects();
	DestroyImageViews();
	DestroyDepthResources();
	DestroyColorResources();
	DestroySwapchainInstance();
}

void Video::RecreateSwapchain()
{
	int width = 0;
	int height = 0;

	glfwGetFramebufferSize(window, &width, &height);

	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	recreateMutex.lock();

	DestroySwapchain();
	CreateSwapchain();

	recreateMutex.unlock();
}

void Video::CreateInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = ApplicationName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(
		&glfwExtensionCount);

	instanceInfo.enabledExtensionCount = glfwExtensionCount;
	instanceInfo.ppEnabledExtensionNames = glfwExtensions;

	if (validate) {
		instanceInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		instanceInfo.enabledLayerCount = 0;
	}

	VkResult res = vkCreateInstance(&instanceInfo, nullptr, &instance);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance");
	}
}

void Video::DestroyInstance()
{
	vkDestroyInstance(instance, nullptr);
}

void Video::CreateSurface()
{
	VkResult res = glfwCreateWindowSurface(
		instance,
		window,
		nullptr,
		&surface);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create surface");
	}
}

void Video::DestroySurface()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

void Video::PickPhysicalDevice()
{
	physicalDevice = VK_NULL_HANDLE;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPU with Vulkan");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (IsDeviceSuitable(device)) {
			physicalDevice = device;
			msaaSamples = GetMaxUsableSampleCount();
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find suitable GPU");
	}
}

void Video::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	std::set<uint32_t> uniqueQueueFamilies = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType =
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount =
		static_cast<uint32_t>(queueCreateInfos.size());
	deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceInfo.pEnabledFeatures = &deviceFeatures;
	deviceInfo.enabledExtensionCount =
		static_cast<uint32_t>(deviceExtensions.size());
	deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (validate) {
		deviceInfo.enabledLayerCount =
			static_cast<uint32_t>(validationLayers.size());
		deviceInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		deviceInfo.enabledLayerCount = 0;
	}

	VkResult res = vkCreateDevice(
		physicalDevice,
		&deviceInfo,
		nullptr,
		&device);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}

	vkGetDeviceQueue(
		device,
		indices.graphicsFamily.value(),
		0,
		&graphicsQueue);

	vkGetDeviceQueue(
		device,
		indices.presentFamily.value(),
		0,
		&presentQueue);
}

void Video::DestroyLogicalDevice()
{
	vkDestroyDevice(device, nullptr);
}

void Video::CreateCommandPools()
{
	QueueFamilyIndices queueFamilyIndices =
		FindQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = 0;

	VkResult res =
		vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool");
	}

	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

	res = vkCreateCommandPool(
		device,
		&poolInfo,
		nullptr,
		&transferCommandPool);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool");
	}
}

void Video::DestroyCommandPools()
{
	vkDestroyCommandPool(device, transferCommandPool, nullptr);
	vkDestroyCommandPool(device, commandPool, nullptr);
}

void Video::CreateSyncObjects()
{
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult res =
		vkCreateFence(device, &fenceInfo, nullptr, &bufferCopyFence);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence");
	}
}

void Video::DestroySyncObjects()
{
	vkDestroyFence(device, bufferCopyFence, nullptr);
}

void Video::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorType =
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> bindings = {
		uboLayoutBinding,
		samplerLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkResult res = vkCreateDescriptorSetLayout(
		device,
		&layoutInfo,
		nullptr,
		&descriptorSetLayout);

	if (res != VK_SUCCESS) {
		throw std::runtime_error(
			"failed to create descriptor set layout");
	}
}

void Video::DestroyDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

void Video::CreateTextureSampler(Model* model)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(model->textureMipLevels);

	VkResult res = vkCreateSampler(
		device,
		&samplerInfo,
		nullptr,
		&model->textureSampler);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler");
	}
}

void Video::DestroyTextureSampler(Model* model)
{
	vkDestroySampler(device, model->textureSampler, nullptr);
}

void Video::CreateTextureSampler(InterfaceObject* object)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(physicalDevice, &properties);

	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = static_cast<float>(object->textureMipLevels);

	VkResult res = vkCreateSampler(
		device,
		&samplerInfo,
		nullptr,
		&object->textureSampler);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler");
	}
}

void Video::DestroyTextureSampler(InterfaceObject* object)
{
	vkDestroySampler(device, object->textureSampler, nullptr);
}

void Video::CreateVertexBuffers()
{
	for (Model* model : models) {
		CreateVertexBuffer(model);
	}

	allowVertexBufferCreation = true;
}

void Video::DestroyVertexBuffers()
{
	allowVertexBufferCreation = false;

	for (Model* model : models) {
		DestroyVertexBuffer(model);
	}
}

void Video::CreateVertexBuffer(Model* model)
{
	VkDeviceSize bufferSize =
		sizeof(Model::Vertex) * model->vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	VkResult res = vkMapMemory(
		device,
		stagingBufferMemory,
		0,
		bufferSize,
		0,
		&data);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("fauled to map memory");
	}

	memcpy(data, model->vertices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		MANAGER_VERTEX,
		model->vertexBuffer,
		model->vertexBufferMemory);

	CopyBuffer(
		stagingBuffer,
		model->vertexBuffer,
		bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Video::DestroyVertexBuffer(Model* model)
{
	vkDestroyBuffer(device, model->vertexBuffer, nullptr);
	vertexBufferMemoryManager->Free(model->vertexBufferMemory);
}

void Video::CreateIndexBuffers()
{
	for (Model* model : models) {
		CreateIndexBuffer(model);
	}

	allowIndexBufferCreation = true;
}

void Video::DestroyIndexBuffers()
{
	allowIndexBufferCreation = false;

	for (Model* model : models) {
		DestroyIndexBuffer(model);
	}
}

void Video::CreateIndexBuffer(Model* model)
{
	VkDeviceSize bufferSize =
		sizeof(Model::VertexIndexType) * model->indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	VkResult res = vkMapMemory(
		device,
		stagingBufferMemory,
		0,
		bufferSize,
		0,
		&data);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to map memory");
	}

	memcpy(data, model->indices.data(), static_cast<size_t>(bufferSize));
	vkUnmapMemory(device, stagingBufferMemory);

	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		MANAGER_INDEX,
		model->indexBuffer,
		model->indexBufferMemory);

	CopyBuffer(
		stagingBuffer,
		model->indexBuffer,
		bufferSize);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Video::DestroyIndexBuffer(Model* model)
{
	vkDestroyBuffer(device, model->indexBuffer, nullptr);
	indexBufferMemoryManager->Free(model->indexBufferMemory);
}

void Video::CreateTextureImages()
{
	for (Model* model : models) {
		CreateTextureImage(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			CreateTextureImage(objectDesc.interfaceObject);
		}
	}

	allowTextureImageCreation = true;
}

void Video::DestroyTextureImages()
{
	allowTextureImageCreation = false;

	for (Model* model : models) {
		DestroyTextureImage(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			DestroyTextureImage(objectDesc.interfaceObject);
		}
	}
}

void Video::CreateTextureImage(Model* model)
{
	int texWidth;
	int texHeight;
	int texChannels;

	stbi_uc* pixels = stbi_load(
		model->textureName.c_str(),
		&texWidth,
		&texHeight,
		&texChannels,
		STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("failed to load texture image");
	}

	model->textureMipLevels = static_cast<uint32_t>(
		std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	VkResult res = vkMapMemory(
		device,
		stagingBufferMemory,
		0,
		imageSize,
		0,
		&data);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to map memory");
	}

	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(
		texWidth,
		texHeight,
		model->textureMipLevels,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		MANAGER_TEXTURE,
		model->textureImage,
		model->textureImageMemory);

	TransitionImageLayout(
		model->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		model->textureMipLevels);

	CopyBufferToImage(
		stagingBuffer,
		model->textureImage,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight));

	GenerateMipmaps(
		model->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		texWidth,
		texHeight,
		model->textureMipLevels);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	model->textureImageView = CreateImageView(
		model->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT,
		model->textureMipLevels);

	CreateTextureSampler(model);
}

void Video::DestroyTextureImage(Model* model)
{
	DestroyTextureSampler(model);
	vkDestroyImageView(device, model->textureImageView, nullptr);
	vkDestroyImage(device, model->textureImage, nullptr);
	textureImageMemoryManager->Free(model->textureImageMemory);
}

void Video::CreateTextureImage(InterfaceObject* object)
{
	int texWidth;
	int texHeight;
	int texChannels;

	stbi_uc* pixels = stbi_load(
		object->textureName.c_str(),
		&texWidth,
		&texHeight,
		&texChannels,
		STBI_rgb_alpha);

	if (!pixels) {
		throw std::runtime_error("failed to load texture image");
	}

	object->textureMipLevels = static_cast<uint32_t>(
		std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

	VkDeviceSize imageSize = texWidth * texHeight * 4;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	CreateBuffer(
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);

	void* data;
	VkResult res = vkMapMemory(
		device,
		stagingBufferMemory,
		0,
		imageSize,
		0,
		&data);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to map memory");
	}

	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	CreateImage(
		texWidth,
		texHeight,
		object->textureMipLevels,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
		VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		MANAGER_TEXTURE,
		object->textureImage,
		object->textureImageMemory);

	TransitionImageLayout(
		object->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		object->textureMipLevels);

	CopyBufferToImage(
		stagingBuffer,
		object->textureImage,
		static_cast<uint32_t>(texWidth),
		static_cast<uint32_t>(texHeight));

	GenerateMipmaps(
		object->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		texWidth,
		texHeight,
		object->textureMipLevels);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);

	object->textureImageView = CreateImageView(
		object->textureImage,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_ASPECT_COLOR_BIT,
		object->textureMipLevels);

	CreateTextureSampler(object);
}

void Video::DestroyTextureImage(InterfaceObject* object)
{
	DestroyTextureSampler(object);
	vkDestroyImageView(device, object->textureImageView, nullptr);
	vkDestroyImage(device, object->textureImage, nullptr);
	textureImageMemoryManager->Free(object->textureImageMemory);
}

void Video::LoadSkybox(std::string fileName)
{
	if (skybox) {
		DestroySkybox();
	}

	skybox = new Model();

	skybox->instances.add();

	skybox->SetTextureName(fileName);
	skybox->instances.front().modelPosition = glm::mat4(1.0f);
	skybox->instances.front().active = true;
	skybox->loaded = false;

	const double h0 = 0.0;
	const double h1 = 1.0 / 3.0;
	const double h2 = 2.0 / 3.0;
	const double h3 = 1.0;

	const double v0 = 0.0;
	const double v1 = 0.25;
	const double v2 = 0.5;
	const double v3 = 0.75;
	const double v4 = 1.0;

	std::vector<Model::Vertex> vertices = {
		// Bottom
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h2}},
		{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h2}},
		{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h3}},
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h3}},

		// Top
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h1}},
		{{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h1}},
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h0}},
		{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h0}},

		// Front
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h2}},
		{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h2}},
		{{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h1}},
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h1}},

		// Back
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v4, h2}},
		{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v3, h2}},
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v3, h1}},
		{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v4, h1}},

		// Left
		{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h2}},
		{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v0, h2}},
		{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v0, h1}},
		{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v1, h1}},

		// Right
		{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h2}},
		{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 0.0f}, {v3, h2}},
		{{1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v3, h1}},
		{{1.0f, -1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {v2, h1}}
	};

	std::vector<Model::VertexIndexType> indices = {
		3, 0, 1, 3, 1, 2,
		5, 4, 7, 5, 7, 6,
		9, 8, 11, 11, 10, 9,
		15, 12, 13, 15, 13, 14,
		19, 16, 17, 19, 17, 18,
		21, 20, 23, 21, 23, 22
	};

	skybox->UpdateBuffers(vertices, indices);

	recreateMutex.lock();

	CreateVertexBuffer(skybox);
	CreateIndexBuffer(skybox);
	CreateTextureImage(skybox);
	CreateUniformBuffers(skybox);
	CreateDescriptorPools(skybox);
	CreateDescriptorSets(skybox);

	recreateMutex.unlock();

	skybox->loaded = true;
}

void Video::DestroySkybox()
{
	if (!skybox) {
		return;
	}

	DestroyDescriptorPools(skybox);
	DestroyUniformBuffers(skybox);
	DestroyTextureImage(skybox);
	DestroyIndexBuffer(skybox);
	DestroyVertexBuffer(skybox);

	delete skybox;
	skybox = nullptr;
}

// Swapchain methods
void Video::CreateSwapchainInstance()
{
	SwapchainSupportDetails swapchainSupport =
		QuerySwapchainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat =
		ChooseSwapchainSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode =
		ChooseSwapchainPresentMode(swapchainSupport.presentModes);
	VkExtent2D extent =
		ChooseSwapchainExtent(swapchainSupport.capabilities);

	uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 7;

	if (swapchainSupport.capabilities.maxImageCount > 0) {
		imageCount = std::clamp(
			imageCount,
			swapchainSupport.capabilities.minImageCount,
			swapchainSupport.capabilities.maxImageCount);
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

	std::vector<uint32_t> queueFamilyIndices = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};

	if (indices.graphicsFamily.value() != indices.presentFamily.value()) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount =
			static_cast<uint32_t>(queueFamilyIndices.size());
		createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform =
		swapchainSupport.capabilities.currentTransform;

	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult res = vkCreateSwapchainKHR(
		device,
		&createInfo,
		nullptr,
		&swapchain);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create swapchain");
	}

	vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
	swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(
		device,
		swapchain,
		&imageCount,
		swapchainImages.data());

	swapchainImageFormat = surfaceFormat.format;
	swapchainExtent = extent;

	if (swapchainImages.size() > 3) {
		maxFramesInFlight = swapchainImages.size() - 3;
	} else {
		maxFramesInFlight = 2;
	}

	currentFrame = 0;
}

void Video::DestroySwapchainInstance()
{
	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Video::CreateColorResources()
{
	VkFormat colorFormat = swapchainImageFormat;

	CreateImage(
		swapchainExtent.width,
		swapchainExtent.height,
		1,
		msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		colorImage,
		colorImageMemory);

	colorImageView = CreateImageView(
		colorImage,
		colorFormat,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1);
}

void Video::DestroyColorResources()
{
	vkDestroyImageView(device, colorImageView, nullptr);
	vkDestroyImage(device, colorImage, nullptr);
	vkFreeMemory(device, colorImageMemory, nullptr);
}

void Video::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	CreateImage(
		swapchainExtent.width,
		swapchainExtent.height,
		1,
		msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthImageMemory);

	depthImageView = CreateImageView(
		depthImage,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		1);

	TransitionImageLayout(
		depthImage,
		depthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		1);
}

void Video::DestroyDepthResources()
{
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);
}

void Video::CreateImageViews()
{
	swapchainImageViews.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); ++i) {
		swapchainImageViews[i] = CreateImageView(
			swapchainImages[i],
			swapchainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1);
	}
}

void Video::DestroyImageViews()
{
	for (auto& imageView : swapchainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}
}

void Video::CreateDescriptorPools()
{
	for (Model* model : models) {
		CreateDescriptorPools(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			CreateDescriptorPool(
				objectDesc.interfaceObject->descriptorPool);
		}
	}

	allowDescriptorPoolCreation = true;
}

void Video::DestroyDescriptorPools()
{
	allowDescriptorPoolCreation = false;
	allowDescriptorSetCreation = false;

	for (Model* model : models) {
		DestroyDescriptorPools(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			DestroyDescriptorPool(
				objectDesc.interfaceObject->descriptorPool);
		}
	}
}

void Video::CreateDescriptorPools(Model* model)
{
	for (auto& instance: model->instances) {
		CreateDescriptorPool(instance.descriptorPool);
	}
}

void Video::DestroyDescriptorPools(Model* model)
{
	for (auto& instance: model->instances) {
		DestroyDescriptorPool(instance.descriptorPool);
	}
}

void Video::CreateDescriptorPool(VkDescriptorPool& descriptorPool)
{
	std::vector<VkDescriptorPoolSize> poolSizes(2);
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount =
		static_cast<uint32_t>(swapchainImages.size());

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount =
		static_cast<uint32_t>(swapchainImages.size());

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount =
		static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(swapchainImages.size());

	VkResult res = vkCreateDescriptorPool(
		device,
		&poolInfo,
		nullptr,
		&descriptorPool);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool");
	}
}

void Video::DestroyDescriptorPool(VkDescriptorPool& descriptorPool)
{
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

void Video::CreateUniformBuffers()
{
	for (Model* model : models) {
		CreateUniformBuffers(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			CreateUniformBuffers(objectDesc.interfaceObject);
		}
	}

	allowUniformBufferCreation = true;
}

void Video::DestroyUniformBuffers()
{
	allowUniformBufferCreation = false;

	for (Model* model : models) {
		DestroyUniformBuffers(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			DestroyUniformBuffers(objectDesc.interfaceObject);
		}
	}
}

void Video::CreateUniformBuffers(Model* model)
{
	for (auto& instance : model->instances) {
		CreateUniformBuffers(instance);
	}
}

void Video::DestroyUniformBuffers(Model* model)
{
	for (auto& instance : model->instances) {
		DestroyUniformBuffers(instance);
	}
}

void Video::CreateUniformBuffers(Model::InstanceDescriptor& instance)
{
	instance.freeMutex.lock();
	instance.free.clear();
	instance.free.resize(swapchainImages.size(), true);
	instance.freeMutex.unlock();

	VkDeviceSize bufferSize = sizeof(Model::UniformBufferObject);

	instance.uniformBuffers.resize(swapchainImages.size());
	instance.uniformBufferMemory.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); ++i) {
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			MANAGER_UNIFORM,
			instance.uniformBuffers[i],
			instance.uniformBufferMemory[i]);
	}
}

void Video::DestroyUniformBuffers(Model::InstanceDescriptor& instance)
{
	for (size_t i = 0; i < swapchainImages.size(); ++i) {
		vkDestroyBuffer(
			device,
			instance.uniformBuffers[i],
			nullptr);

		uniformBufferMemoryManager->Free(
			instance.uniformBufferMemory[i]);
	}
}

void Video::CreateUniformBuffers(InterfaceObject* object)
{
	object->freeMutex.lock();
	object->free.clear();
	object->free.resize(swapchainImages.size(), true);
	object->freeMutex.unlock();

	VkDeviceSize bufferSize = sizeof(InterfaceObject::Area);

	object->uniformBuffers.resize(swapchainImages.size());
	object->uniformBufferMemory.resize(swapchainImages.size());

	for (size_t i = 0; i < swapchainImages.size(); ++i) {
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			MANAGER_UNIFORM,
			object->uniformBuffers[i],
			object->uniformBufferMemory[i]);
	}
}

void Video::DestroyUniformBuffers(InterfaceObject* object)
{
	for (size_t i = 0; i < swapchainImages.size(); ++i) {
		vkDestroyBuffer(
			device,
			object->uniformBuffers[i],
			nullptr);

		uniformBufferMemoryManager->Free(
			object->uniformBufferMemory[i]);
	}
}

void Video::CreateDescriptorSets()
{
	for (Model* model : models) {
		CreateDescriptorSets(model);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		if (objectDesc.interfaceObject->visual) {
			CreateDescriptorSets(objectDesc.interfaceObject);
		}
	}

	allowDescriptorSetCreation = true;
}

void Video::CreateDescriptorSets(Model* model)
{
	for (auto& instance : model->instances) {
		CreateDescriptorSets(model, instance);
	}
}

void Video::CreateDescriptorSets(
	Model* model,
	Model::InstanceDescriptor& instance)
{
	std::vector<VkDescriptorSetLayout> layouts(
		swapchainImages.size(),
		descriptorSetLayout);

	instance.descriptorSets.resize(swapchainImages.size());

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = instance.descriptorPool;
	allocInfo.descriptorSetCount =
		static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	VkResult res = vkAllocateDescriptorSets(
		device,
		&allocInfo,
		instance.descriptorSets.data());

	if (res != VK_SUCCESS) {
		throw std::runtime_error(
			"failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < instance.descriptorSets.size(); ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = instance.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(Model::UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout =
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = model->textureImageView;
		imageInfo.sampler = model->textureSampler;

		std::vector<VkWriteDescriptorSet> descriptorWrites(2);

		descriptorWrites[0].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = instance.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType =
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = instance.descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pBufferInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}

void Video::CreateDescriptorSets(InterfaceObject* object)
{
	std::vector<VkDescriptorSetLayout> layouts(
		swapchainImages.size(),
		descriptorSetLayout);

	object->descriptorSets.resize(swapchainImages.size());

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType =
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = object->descriptorPool;
	allocInfo.descriptorSetCount =
		static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();

	VkResult res = vkAllocateDescriptorSets(
		device,
		&allocInfo,
		object->descriptorSets.data());

	if (res != VK_SUCCESS) {
		throw std::runtime_error(
			"failed to allocate descriptor sets");
	}

	for (size_t i = 0; i < object->descriptorSets.size(); ++i) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = object->uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(InterfaceObject::Area);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout =
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = object->textureImageView;
		imageInfo.sampler = object->textureSampler;

		std::vector<VkWriteDescriptorSet> descriptorWrites(2);

		descriptorWrites[0].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = object->descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType =
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = nullptr;
		descriptorWrites[0].pTexelBufferView = nullptr;

		descriptorWrites[1].sType =
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = object->descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType =
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pBufferInfo = nullptr;
		descriptorWrites[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(
			device,
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}

void Video::CreateRenderPass()
{
	CreateObjectRenderPass();
	CreateSkyboxRenderPass();
	CreateInterfaceRenderPass();
}

void Video::CreateObjectRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = msaaSamples;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = msaaSamples;

	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout =
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = nullptr;

	std::vector<VkAttachmentDescription> attachments = {
		colorAttachment,
		depthAttachment,
	};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount =
		static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();

	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(
		device,
		&renderPassInfo,
		nullptr,
		&renderPass);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void Video::CreateSkyboxRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = msaaSamples;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.pResolveAttachments = nullptr;

	std::vector<VkAttachmentDescription> attachments = {
		colorAttachment
	};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount =
		static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();

	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(
		device,
		&renderPassInfo,
		nullptr,
		&skyboxRenderPass);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void Video::CreateInterfaceRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapchainImageFormat;
	colorAttachment.samples = msaaSamples;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapchainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp =
		VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 1;
	colorAttachmentResolveRef.layout =
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	std::vector<VkAttachmentDescription> attachments = {
		colorAttachment,
		colorAttachmentResolve
	};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount =
		static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();

	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VkResult res = vkCreateRenderPass(
		device,
		&renderPassInfo,
		nullptr,
		&interfaceRenderPass);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass");
	}
}

void Video::DestroyRenderPass()
{
	vkDestroyRenderPass(device, renderPass, nullptr);
	vkDestroyRenderPass(device, skyboxRenderPass, nullptr);
	vkDestroyRenderPass(device, interfaceRenderPass, nullptr);
}

void Video::CreateGraphicsPipeline()
{
	CreateObjectGraphicsPipeline();
	CreateSkyboxGraphicsPipeline();
	CreateInterfaceGraphicsPipeline();
}

void Video::CreateObjectGraphicsPipeline()
{
	VkShaderModule vertShaderModule =
		CreateShaderModule(
			sizeof(ObjectVertexShader),
			ObjectVertexShader);
	VkShaderModule fragShaderModule =
		CreateShaderModule(
			sizeof(ObjectFragmentShader),
			ObjectFragmentShader);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	auto bindingDescription = Model::Vertex::GetBindingDescription();
	auto attributeDescriptions =
		Model::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount =
		static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions =
		attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(swapchainExtent.width);
	viewport.height = float(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult res = vkCreatePipelineLayout(
		device,
		&pipelineLayoutInfo,
		nullptr,
		&pipelineLayout);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType =
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount =
		static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&graphicsPipeline);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Video::CreateSkyboxGraphicsPipeline()
{
	VkShaderModule vertShaderModule =
		CreateShaderModule(
			sizeof(ObjectVertexShader),
			ObjectVertexShader);
	VkShaderModule fragShaderModule =
		CreateShaderModule(
			sizeof(ObjectFragmentShader),
			ObjectFragmentShader);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	auto bindingDescription = Model::Vertex::GetBindingDescription();
	auto attributeDescriptions =
		Model::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount =
		static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions =
		attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(swapchainExtent.width);
	viewport.height = float(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult res = vkCreatePipelineLayout(
		device,
		&pipelineLayoutInfo,
		nullptr,
		&skyboxPipelineLayout);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType =
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount =
		static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = skyboxPipelineLayout;
	pipelineInfo.renderPass = skyboxRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&skyboxGraphicsPipeline);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Video::CreateInterfaceGraphicsPipeline()
{
	VkShaderModule vertShaderModule =
		CreateShaderModule(
			sizeof(InterfaceVertexShader),
			InterfaceVertexShader);
	VkShaderModule fragShaderModule =
		CreateShaderModule(
			sizeof(InterfaceFragmentShader),
			InterfaceFragmentShader);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {
		vertShaderStageInfo,
		fragShaderStageInfo
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType =
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(swapchainExtent.width);
	viewport.height = float(swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType =
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType =
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType =
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = msaaSamples;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT |
		VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT |
		VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor =
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType =
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType =
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	VkResult res = vkCreatePipelineLayout(
		device,
		&pipelineLayoutInfo,
		nullptr,
		&interfacePipelineLayout);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout");
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType =
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_FALSE;
	depthStencil.depthWriteEnable = VK_FALSE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType =
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount =
		static_cast<uint32_t>(shaderStages.size());
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = interfacePipelineLayout;
	pipelineInfo.renderPass = interfaceRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(
		device,
		VK_NULL_HANDLE,
		1,
		&pipelineInfo,
		nullptr,
		&interfaceGraphicsPipeline);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void Video::DestroyGraphicsPipeline()
{
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	vkDestroyPipeline(device, skyboxGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
	vkDestroyPipeline(device, interfaceGraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, interfacePipelineLayout, nullptr);
}

void Video::CreateFramebuffers()
{
	CreateObjectFramebuffers();
	CreateSkyboxFramebuffers();
	CreateInterfaceFramebuffers();
}

void Video::CreateObjectFramebuffers()
{
	swapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
		std::vector<VkImageView> attachments = {
			colorImageView,
			depthImageView,
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType =
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount =
			static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		VkResult res = vkCreateFramebuffer(
			device,
			&framebufferInfo,
			nullptr,
			&swapchainFramebuffers[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create framebuffer");
		}
	}
}

void Video::CreateSkyboxFramebuffers()
{
	skyboxSwapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
		std::vector<VkImageView> attachments = {
			colorImageView,
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType =
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = skyboxRenderPass;
		framebufferInfo.attachmentCount =
			static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		VkResult res = vkCreateFramebuffer(
			device,
			&framebufferInfo,
			nullptr,
			&skyboxSwapchainFramebuffers[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create framebuffer");
		}
	}
}

void Video::CreateInterfaceFramebuffers()
{
	interfaceSwapchainFramebuffers.resize(swapchainImageViews.size());

	for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
		std::vector<VkImageView> attachments = {
			colorImageView,
			swapchainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType =
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = interfaceRenderPass;
		framebufferInfo.attachmentCount =
			static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapchainExtent.width;
		framebufferInfo.height = swapchainExtent.height;
		framebufferInfo.layers = 1;

		VkResult res = vkCreateFramebuffer(
			device,
			&framebufferInfo,
			nullptr,
			&interfaceSwapchainFramebuffers[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to create framebuffer");
		}
	}
}

void Video::DestroyFramebuffers()
{
	for (auto framebuffer : swapchainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (auto framebuffer : skyboxSwapchainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (auto framebuffer : interfaceSwapchainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}
}

void Video::CreateSwapchainSyncObjects()
{
	imageAvailableSemaphores.resize(maxFramesInFlight);
	renderFinishedSemaphores.resize(maxFramesInFlight);
	inFlightFences.resize(maxFramesInFlight);
	imagesInFlight.clear();
	imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		VkResult res = vkCreateFence(
			device,
			&fenceInfo,
			nullptr,
			&inFlightFences[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create fence");
		}

		res = vkCreateSemaphore(
			device,
			&semaphoreInfo,
			nullptr,
			&imageAvailableSemaphores[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphore");
		}

		res = vkCreateSemaphore(
			device,
			&semaphoreInfo,
			nullptr,
			&renderFinishedSemaphores[i]);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to create semaphore");
		}
	}
}

void Video::DestroySwapchainSyncObjects()
{
	for (size_t i = 0; i < maxFramesInFlight; ++i) {
		vkDestroyFence(device, inFlightFences[i], nullptr);
		vkDestroySemaphore(
			device,
			imageAvailableSemaphores[i],
			nullptr);
		vkDestroySemaphore(
			device,
			renderFinishedSemaphores[i],
			nullptr);
	}
}

void Video::CreateCommandBuffers()
{
	commandBuffers.resize(swapchainImages.size(), VK_NULL_HANDLE);
}

void Video::DestroyCommandBuffers()
{
	for (auto commandBuffer : commandBuffers) {
		if (commandBuffer != VK_NULL_HANDLE) {
			vkFreeCommandBuffers(
				device,
				commandPool,
				1,
				&commandBuffer);
		}
	}

	commandBuffers.clear();
}

// Rendering
void Video::DrawFrame()
{
	uint32_t imageIndex;

	vkWaitForFences(
		device,
		1,
		&inFlightFences[currentFrame],
		VK_TRUE,
		UINT64_MAX);

	VkResult res = vkAcquireNextImageKHR(
		device,
		swapchain,
		UINT64_MAX,
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	if (res != VK_SUCCESS) {
		if (res == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapchain();
			return;
		} else if (res != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error(
				"failed to acquire next image");
		}
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(
			device,
			1,
			&imagesInFlight[imageIndex],
			VK_TRUE,
			UINT64_MAX);
	}

	drawMutex.lock();

	UpdateUniformBuffers(imageIndex);

	CreateCommandBuffer(imageIndex);

	drawMutex.unlock();

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {
		imageAvailableSemaphores[currentFrame]
	};

	VkSemaphore signalSemaphores[] = {
		renderFinishedSemaphores[currentFrame]
	};

	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(device, 1, &inFlightFences[currentFrame]);

	res = vkQueueSubmit(
		graphicsQueue,
		1,
		&submitInfo,
		inFlightFences[currentFrame]);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to submit command buffer");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapchains[] = {swapchain};

	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapchains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	res = vkQueuePresentKHR(presentQueue, &presentInfo);

	if (res != VK_SUCCESS || framebufferResized) {
		if (res == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized) {
			framebufferResized = false;
			RecreateSwapchain();
		} else if (res != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error(
				"failed to present swapchain image");
		}
	}

	currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

void Video::MainLoop()
{
	while (work && !glfwWindowShouldClose(window)) {
		glfwPollEvents();
		DrawFrame();
	}

	vkDeviceWaitIdle(device);
}

// Helper functions
std::vector<const char*> Video::GetValidationLayers()
{
	std::vector<std::vector<const char*>> layerGroups;

	std::vector<const char*> groupLunarG;
	groupLunarG.push_back("VK_LAYER_LUNARG_core_validation");
	groupLunarG.push_back("VK_LAYER_LUNARG_parameter_validation");
	groupLunarG.push_back("VK_LAYER_LUNARG_standard_validation");
	layerGroups.push_back(groupLunarG);

	std::vector<const char*> groupKhronos;
	groupKhronos.push_back("VK_LAYER_KHRONOS_validation");
	layerGroups.push_back(groupKhronos);

	uint32_t availableLayerCount;
	vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);

	if (availableLayerCount == 0) {
		throw std::runtime_error("no validation layers found");
	}

	std::vector<VkLayerProperties> availableLayers(availableLayerCount);
	vkEnumerateInstanceLayerProperties(
		&availableLayerCount,
		availableLayers.data());

	for (const auto& group : layerGroups) {
		bool groupAvailable = true;

		for (const char* layerName : group) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				int cmp = strcmp(
					layerName,
					layerProperties.layerName);

				if (cmp == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				groupAvailable = false;
			}
		}

		if (groupAvailable) {
			return group;
		}
	}

	throw std::runtime_error("failed to find validation layers");
}

bool Video::IsDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapchainAdequate = false;

	if (extensionsSupported) {
		SwapchainSupportDetails swapchainSupport =
			QuerySwapchainSupport(device);

		swapchainAdequate = !swapchainSupport.formats.empty() &&
			!swapchainSupport.presentModes.empty();
	}

	bool res = deviceProperties.deviceType ==
		VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		extensionsSupported &&
		swapchainAdequate &&
		deviceFeatures.geometryShader &&
		deviceFeatures.samplerAnisotropy &&
		FindQueueFamilies(device).graphicsFamily.has_value() &&
		FindQueueFamilies(device).presentFamily.has_value();

	return res;
}

bool Video::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(
		device,
		nullptr,
		&extensionCount,
		nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(
		device,
		nullptr,
		&extensionCount,
		availableExtensions.data());

	for (const char* requiredExtension : deviceExtensions) {
		bool extensionFound = false;

		for (const auto& extension : availableExtensions) {
			bool cmp = strcmp(
				requiredExtension,
				extension.extensionName);

			if (cmp == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound) {
			return false;
		}
	}

	return true;
}

Video::SwapchainSupportDetails Video::QuerySwapchainSupport(
	VkPhysicalDevice device)
{
	SwapchainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		device,
		surface,
		&details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(
		device,
		surface,
		&formatCount,
		nullptr);

	if (formatCount > 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device,
			surface,
			&formatCount,
			details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(
		device,
		surface,
		&presentModeCount,
		nullptr);

	if (presentModeCount > 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			surface,
			&presentModeCount,
			details.presentModes.data());
	}

	return details;
}

Video::QueueFamilyIndices Video::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
		device,
		&queueFamilyCount,
		nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(
		device,
		&queueFamilyCount,
		queueFamilies.data());

	int i = 0;

	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(
			device,
			i,
			surface,
			&presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		++i;
	}

	return indices;
}

void Video::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	Video* video = reinterpret_cast<Video*>(
		glfwGetWindowUserPointer(window));
	video->framebufferResized = true;
}

void Video::KeyActionCallback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	Video* video = reinterpret_cast<Video*>(
		glfwGetWindowUserPointer(window));

	video->keyBindMutex.lock();

	for (const auto& keyBinding : video->keyBindings) {
		if (keyBinding.key == key && keyBinding.action == action) {
			keyBinding.callback(key, action, keyBinding.object);
		}
	}

	video->keyBindMutex.unlock();
}

void Video::CursorMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
	Video* video = reinterpret_cast<Video*>(
		glfwGetWindowUserPointer(window));

	video->cursorX = xpos;
	video->cursorY = ypos;

	float X = video->cursorX / video->swapchainExtent.width * 2.0 - 1.0;
	float Y = video->cursorY / video->swapchainExtent.height * 2.0 - 1.0;

	if (!video->cameraCursor) {
		for (const InterfaceObjectDescriptor& objectDesc :
			video->interfaceObjects)
		{
			InterfaceObject* object = objectDesc.interfaceObject;

			if (!object->active) {
				continue;
			}

			bool inArea =
				X >= object->area.x0 &&
				X <= object->area.x1 &&
				Y >= object->area.y0 &&
				Y <= object->area.y1;

			if (inArea) {
				float localX = X - object->area.x0;
				localX /= abs(
					object->area.x1 - object->area.x0);
				localX = localX * 2.0 - 1.0;

				float localY = Y - object->area.y0;
				localY /= abs(
					object->area.y1 - object->area.y0);
				localY = localY * 2.0 - 1.0;

				bool passAction =
					object->CursorMove(
						localX,
						localY,
						true);

				if (!passAction) {
					return;
				}
			} else {
				object->CursorMove(0, 0, false);
			}
		}
	}

	if (video->cursorMoveCallback) {
		video->cursorMoveCallback(
			xpos,
			ypos,
			video->cursorMoveController);
	}
}

void Video::MouseButtonCallback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	Video* video = reinterpret_cast<Video*>(
		glfwGetWindowUserPointer(window));

	float X = video->cursorX / video->swapchainExtent.width * 2.0 - 1.0;
	float Y = video->cursorY / video->swapchainExtent.height * 2.0 - 1.0;

	if (!video->cameraCursor) {
		for (const InterfaceObjectDescriptor& objectDesc :
			video->interfaceObjects)
		{
			InterfaceObject* object = objectDesc.interfaceObject;

			if (!object->active) {
				continue;
			}

			bool inArea =
				X >= object->area.x0 &&
				X <= object->area.x1 &&
				Y >= object->area.y0 &&
				Y <= object->area.y1;

			if (inArea) {
				bool passAction =
					object->MouseButton(button, action);

				if (!passAction) {
					return;
				}
			}
		}
	}

	if (video->mouseButtonCallback) {
		video->mouseButtonCallback(
			button,
			action,
			video->mouseButtonController);
	}
}

void Video::ScrollCallback(
	GLFWwindow* window,
	double xoffset,
	double yoffset)
{
	Video* video = reinterpret_cast<Video*>(
		glfwGetWindowUserPointer(window));

	float X = video->cursorX / video->swapchainExtent.width * 2.0 - 1.0;
	float Y = video->cursorY / video->swapchainExtent.height * 2.0 - 1.0;

	if (!video->cameraCursor) {
		for (const InterfaceObjectDescriptor& objectDesc :
			video->interfaceObjects)
		{
			InterfaceObject* object = objectDesc.interfaceObject;

			if (!object->active) {
				continue;
			}

			bool inArea =
				X >= object->area.x0 &&
				X <= object->area.x1 &&
				Y >= object->area.y0 &&
				Y <= object->area.y1;

			if (inArea) {
				bool passAction =
					object->Scroll(xoffset, yoffset);

				if (!passAction) {
					return;
				}
			}
		}
	}

	if (video->scrollCallback) {
		video->scrollCallback(
			xoffset,
			yoffset,
			video->scrollController);
	}
}

VkSurfaceFormatKHR Video::ChooseSwapchainSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		bool accept =
			availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace ==
			VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		if (accept) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Video::ChooseSwapchainPresentMode(
	const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Video::ChooseSwapchainExtent(
	const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	} else {
		int width;
		int height;

		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(
				actualExtent.width,
				capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(
				actualExtent.height,
				capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Video::CreateImage(
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels,
	VkSampleCountFlagBits numSamples,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = numSamples;
	imageInfo.flags = 0;

	VkResult res = vkCreateImage(device, &imageInfo, nullptr, &image);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(
		memRequirements.memoryTypeBits,
		properties);

	res = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

uint32_t Video::FindMemoryType(
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
		bool accept = (typeFilter & (1 << i)) &&
			(memProperties.memoryTypes[i].propertyFlags &
			properties) == properties;

		if (accept) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type");
}

VkFormat Video::FindDepthFormat()
{
	return FindSupportedFormat(
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT);
}

VkFormat Video::FindSupportedFormat(
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(
			physicalDevice,
			format,
			&props);

		bool success =
			(tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) ||
			(tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features);

		if (success) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format");
}

VkImageView Video::CreateImageView(
	VkImage image,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;

	VkResult res;
	res = vkCreateImageView(device, &viewInfo, nullptr, &imageView);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create image view");
	}

	return imageView;
}

void Video::TransitionImageLayout(
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |=
				VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask =
			VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask =
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::runtime_error("unsupported layout transition type");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage,
		destinationStage,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	EndSingleTimeCommands(commandBuffer);
}

VkCommandBuffer Video::BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Video::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkResetFences(device, 1, &bufferCopyFence);
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, bufferCopyFence);
	vkWaitForFences(device, 1, &bufferCopyFence, VK_TRUE, UINT64_MAX);

	vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);
}

bool Video::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Video::CreateBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	GPUMemoryManagerType managerType,
	VkBuffer& buffer,
	GPUMemoryManager::MemoryAllocationProperties& memoryAllocation)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	GPUMemoryManager* memoryManager;

	if (managerType == MANAGER_VERTEX) {
		if (!vertexBufferMemoryManager) {
			vertexBufferMemoryManager = new GPUMemoryManager(
				device,
				8192,
				FindMemoryType(
					memRequirements.memoryTypeBits,
					properties),
				memRequirements.alignment);
		}

		memoryManager = vertexBufferMemoryManager;
	} else if (managerType == MANAGER_INDEX) {
		if (!indexBufferMemoryManager) {
			indexBufferMemoryManager = new GPUMemoryManager(
				device,
				8192,
				FindMemoryType(
					memRequirements.memoryTypeBits,
					properties),
				memRequirements.alignment);
		}

		memoryManager = indexBufferMemoryManager;
	} else if (managerType == MANAGER_UNIFORM) {
		if (!uniformBufferMemoryManager) {
			uniformBufferMemoryManager = new GPUMemoryManager(
				device,
				8192,
				FindMemoryType(
					memRequirements.memoryTypeBits,
					properties),
				memRequirements.alignment);
		}

		memoryManager = uniformBufferMemoryManager;
	} else {
		throw std::runtime_error("unsupported memory manager called");
	}

	memoryAllocation = memoryManager->Allocate(memRequirements.size);

	vkBindBufferMemory(
		device,
		buffer,
		memoryAllocation.memory,
		memoryAllocation.offset);
}

void Video::CreateBuffer(
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkBuffer& buffer,
	VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(
		memRequirements.memoryTypeBits,
		properties);

	res = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Video::CopyBuffer(
	VkBuffer srcBuffer,
	VkBuffer dstBuffer,
	VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void Video::CreateImage(
	uint32_t width,
	uint32_t height,
	uint32_t mipLevels,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	GPUMemoryManagerType managerType,
	VkImage& image,
	GPUMemoryManager::MemoryAllocationProperties& memoryAllocation)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	VkResult res = vkCreateImage(device, &imageInfo, nullptr, &image);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create image");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	GPUMemoryManager* memoryManager;

	if (managerType == MANAGER_TEXTURE) {
		if (!textureImageMemoryManager) {
			textureImageMemoryManager = new GPUMemoryManager(
				device,
				65536,
				FindMemoryType(
					memRequirements.memoryTypeBits,
					properties),
				memRequirements.alignment);
		}

		memoryManager = textureImageMemoryManager;
	} else {
		throw std::runtime_error("unsupported memory manager called");
	}

	memoryAllocation = memoryManager->Allocate(memRequirements.size);

	vkBindImageMemory(
		device,
		image,
		memoryAllocation.memory,
		memoryAllocation.offset);
}

void Video::CopyBufferToImage(
	VkBuffer buffer,
	VkImage image,
	uint32_t width,
	uint32_t height)
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);

	EndSingleTimeCommands(commandBuffer);
}

VkShaderModule Video::CreateShaderModule(uint32_t size, const uint32_t* code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = code;

	VkShaderModule shaderModule;

	VkResult res = vkCreateShaderModule(
		device,
		&createInfo,
		nullptr,
		&shaderModule);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module");
	}

	return shaderModule;
}

void Video::CreateCommandBuffer(uint32_t imageIndex)
{
	if (commandBuffers[imageIndex] != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(
			device,
			commandPool,
			1,
			&commandBuffers[imageIndex]);
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;

	VkResult res = vkAllocateCommandBuffers(
		device,
		&allocInfo,
		&commandBuffer);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffer");
	}

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	res = vkBeginCommandBuffer(commandBuffer, &beginInfo);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to begin command buffer");
	}

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = skyboxRenderPass;
		renderPassInfo.framebuffer =
			skyboxSwapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;

		VkClearValue clearValues[1];

		clearValues[0].color = {1.0f, 1.0f, 1.0f, 1.0f};

		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skyboxGraphicsPipeline);
	}

	if (skybox && skybox->instances.front().active) {
		VkBuffer vertexBuffers[] = {skybox->vertexBuffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindVertexBuffers(
			commandBuffer,
			0,
			1,
			vertexBuffers,
			offsets);

		vkCmdBindIndexBuffer(
			commandBuffer,
			skybox->indexBuffer,
			0,
			VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skyboxPipelineLayout,
			0,
			1,
			&skybox->instances.front().descriptorSets[imageIndex],
			0,
			nullptr);

		vkCmdDrawIndexed(
			commandBuffer,
			static_cast<uint32_t>(skybox->indices.size()),
			1,
			0,
			0,
			0);
	}

	vkCmdEndRenderPass(commandBuffer);

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;

		VkClearValue clearValues[2];
		clearValues[1].depthStencil = {1.0f, 0};

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			graphicsPipeline);
	}

	for (Model* model : models) {
		VkBuffer vertexBuffers[] = {model->vertexBuffer};
		VkDeviceSize offsets[] = {0};

		vkCmdBindVertexBuffers(
			commandBuffer,
			0,
			1,
			vertexBuffers,
			offsets);

		vkCmdBindIndexBuffer(
			commandBuffer,
			model->indexBuffer,
			0,
			VK_INDEX_TYPE_UINT32);

		for (auto& instance : model->instances) {
			if (!instance.active) {
				instance.freeMutex.lock();
				instance.free[imageIndex] = true;
				instance.freeMutex.unlock();
				continue;
			}

			instance.freeMutex.lock();
			instance.free[imageIndex] = false;
			instance.freeMutex.unlock();

			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				0,
				1,
				&instance.descriptorSets[imageIndex],
				0,
				nullptr);

			vkCmdDrawIndexed(
				commandBuffer,
				static_cast<uint32_t>(model->indices.size()),
				instance.instanceCount,
				0,
				0,
				0);
		}
	}

	vkCmdEndRenderPass(commandBuffer);

	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = interfaceRenderPass;
		renderPassInfo.framebuffer =
			interfaceSwapchainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainExtent;

		renderPassInfo.clearValueCount = 0;
		renderPassInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(
			commandBuffer,
			&renderPassInfo,
			VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			interfaceGraphicsPipeline);
	}

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		InterfaceObject* object = objectDesc.interfaceObject;

		if (!object->visual) {
			continue;
		}

		if (!object->active) {
			object->freeMutex.lock();
			object->free[imageIndex] = true;
			object->freeMutex.unlock();
			continue;
		}

		object->freeMutex.lock();
		object->free[imageIndex] = false;
		object->freeMutex.unlock();

		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&object->descriptorSets[imageIndex],
			0,
			nullptr);

		vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	}

	vkCmdEndRenderPass(commandBuffer);

	res = vkEndCommandBuffer(commandBuffer);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to end command buffer");
	}

	commandBuffers[imageIndex] = commandBuffer;
}

void Video::UpdateUniformBuffers(uint32_t imageIndex)
{
	cameraMutex.lock();

	Model::UniformBufferObject ubo;

	ubo.proj = glm::perspective(
		glm::radians(FOV),
		float(swapchainExtent.width) /
		float(swapchainExtent.height),
		0.1f,
		100.0f);
	ubo.proj[1][1] *= -1;

	ubo.view = glm::lookAt(
		camera.position,
		camera.target,
		camera.up);

	if (skybox) {
		auto& skyboxInstance = skybox->instances.front();

		ubo.model = glm::translate(
			skyboxInstance.modelPosition,
			camera.position);

		void* data;
		VkResult res = vkMapMemory(
			device,
			skyboxInstance.uniformBufferMemory[imageIndex].memory,
			skyboxInstance.uniformBufferMemory[imageIndex].offset,
			sizeof(ubo),
			0,
			&data);

		if (res != VK_SUCCESS) {
			throw std::runtime_error("failed to map memory");
		}

		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(
			device, 
			skyboxInstance.uniformBufferMemory[imageIndex].memory);
	}

	for (Model* model : models) {
		for (auto& instance : model->instances) {
			if (!instance.active) {
				continue;
			}

			instance.mpMutex.lock();
			ubo.model = instance.modelPosition;
			instance.mpMutex.unlock();

			void* data;
			VkResult res = vkMapMemory(
				device,
				instance.uniformBufferMemory[imageIndex].memory,
				instance.uniformBufferMemory[imageIndex].offset,
				sizeof(ubo),
				0,
				&data);

			if (res != VK_SUCCESS) {
				throw std::runtime_error(
					"failed to map memory");
			}

			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(
				device, 
				instance.uniformBufferMemory[imageIndex].memory);
		}
	}

	cameraMutex.unlock();

	for (const InterfaceObjectDescriptor& objectDesc : interfaceObjects) {
		InterfaceObject* object = objectDesc.interfaceObject;

		if (!object->active || !object->visual) {
			continue;
		}

		void* data;
		VkResult res = vkMapMemory(
			device,
			object->uniformBufferMemory[imageIndex].memory,
			object->uniformBufferMemory[imageIndex].offset,
			sizeof(InterfaceObject::Area),
			0,
			&data);

		if (res != VK_SUCCESS) {
			throw std::runtime_error(
				"failed to map memory");
		}

		memcpy(data, &object->area, sizeof(InterfaceObject::Area));
		vkUnmapMemory(
			device, 
			object->uniformBufferMemory[imageIndex].memory);
	}
}

namespace std
{
	template<>
	struct hash<Model::Vertex>
	{
		size_t operator()(const Model::Vertex& vertex) const
		{
			return
				((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

void Video::LoadModelFromObj(Model* model, const std::string& fileName)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn;
	std::string err;

	bool success = tinyobj::LoadObj(
		&attrib,
		&shapes,
		&materials, &warn,
		&err,
		fileName.c_str());

	if (!success) {
		throw std::runtime_error(warn + err);
	}

	model->vertices.clear();
	model->indices.clear();

	std::unordered_map<Model::Vertex, Model::VertexIndexType>
		uniqueVertices;

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Model::Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index],
				1.0 - attrib.texcoords[2 *
					index.texcoord_index + 1]
			};

			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] =
					static_cast<Model::VertexIndexType>(
						model->vertices.size());
				model->vertices.push_back(vertex);
			}

			model->indices.push_back(uniqueVertices[vertex]);
		}
	}
}

void Video::GenerateMipmaps(
	VkImage image,
	VkFormat imageFormat,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(
		physicalDevice,
		imageFormat,
		&formatProperties);

	bool success = formatProperties.optimalTilingFeatures &
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	if (!success) {
		throw std::runtime_error(
			"image format does not support linear blitting");
	}

	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	uint32_t mipWidth = texWidth;
	uint32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; ++i) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = {0, 0, 0};
		blit.srcOffsets[1] = {
			static_cast<int32_t>(mipWidth),
			static_cast<int32_t>(mipHeight),
			1
		};

		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;

		blit.dstOffsets[0] = {0, 0, 0};
		blit.dstOffsets[1] = {
			static_cast<int32_t>(mipWidth > 1 ? mipWidth / 2 : 1),
			static_cast<int32_t>(mipHeight > 1 ? mipHeight / 2 : 1),
			1
		};

		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(
			commandBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0,
			nullptr,
			0,
			nullptr,
			1,
			&barrier);

		if (mipWidth > 1) {
			mipWidth /= 2;
		}

		if (mipHeight > 1) {
			mipHeight /= 2;
		}
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		0,
		nullptr,
		1,
		&barrier);

	EndSingleTimeCommands(commandBuffer);
}

VkSampleCountFlagBits Video::GetMaxUsableSampleCount()
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(
		physicalDevice,
		&physicalDeviceProperties);

	VkSampleCountFlags counts =
		physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	std::vector<VkSampleCountFlagBits> bits = {
		VK_SAMPLE_COUNT_64_BIT,
		VK_SAMPLE_COUNT_32_BIT,
		VK_SAMPLE_COUNT_16_BIT,
		VK_SAMPLE_COUNT_8_BIT,
		VK_SAMPLE_COUNT_4_BIT,
		VK_SAMPLE_COUNT_2_BIT
	};

	for (auto bit : bits) {
		if (counts & bit) {
			return bit;
		}
	}

	return VK_SAMPLE_COUNT_1_BIT;
}

// Public methods
Model* Video::CreateModel(std::string* fileName)
{
	Model* model = new Model();

	if (fileName) {
		LoadModelFromObj(model, *fileName);
	}

	model->loaded = false;

	return model;
}

void Video::DestroyModel(Model* model)
{
	delete model;
}

void Video::LoadModel(Model* model)
{
	recreateMutex.lock();
	drawMutex.lock();

	if (allowVertexBufferCreation) {
		CreateVertexBuffer(model);
	}

	if (allowIndexBufferCreation) {
		CreateIndexBuffer(model);
	}

	if (allowTextureImageCreation) {
		CreateTextureImage(model);
	}

	if (allowUniformBufferCreation) {
		CreateUniformBuffers(model);
	}

	if (allowDescriptorPoolCreation) {
		CreateDescriptorPools(model);
	}

	if (allowDescriptorSetCreation) {
		CreateDescriptorSets(model);
	}

	model->loaded = true;

	models.insert(model);

	drawMutex.unlock();
	recreateMutex.unlock();
}

void Video::UnloadModel(Model* model)
{
	while (model->instances.size() > 0) {
		RemoveInstance(model, 0);
	}

	recreateMutex.lock();
	drawMutex.lock();

	if (allowDescriptorPoolCreation) {
		DestroyDescriptorPools(model);
	}

	if (allowUniformBufferCreation) {
		DestroyUniformBuffers(model);
	}

	if (allowTextureImageCreation) {
		DestroyTextureImage(model);
	}

	if (allowIndexBufferCreation) {
		DestroyIndexBuffer(model);
	}

	if (allowVertexBufferCreation) {
		DestroyVertexBuffer(model);
	}

	model->loaded = false;

	models.erase(model);

	drawMutex.unlock();
	recreateMutex.unlock();
}

void Video::LoadInterface(InterfaceObject* object)
{
	recreateMutex.lock();
	drawMutex.lock();

	if (object->visual) {
		if (allowTextureImageCreation) {
			CreateTextureImage(object);
		}

		if (allowUniformBufferCreation) {
			CreateUniformBuffers(object);
		}

		if (allowDescriptorPoolCreation) {
			CreateDescriptorPool(object->descriptorPool);
		}

		if (allowDescriptorSetCreation) {
			CreateDescriptorSets(object);
		}
	}

	object->loaded = true;

	interfaceObjects.insert(
		InterfaceObjectDescriptor(object, object->depth));

	drawMutex.unlock();
	recreateMutex.unlock();
}

void Video::UnloadInterface(InterfaceObject* object)
{
	bool ready;

	object->active = false;

	do {
		ready = true;

		object->freeMutex.lock();

		for (bool val : object->free) {
			ready = ready && val;
		}

		object->freeMutex.unlock();
	} while (work && !ready);

	recreateMutex.lock();
	drawMutex.lock();

	if (object->visual) {
		if (allowDescriptorPoolCreation) {
			DestroyDescriptorPool(object->descriptorPool);
		}

		if (allowUniformBufferCreation) {
			DestroyUniformBuffers(object);
		}

		if (allowTextureImageCreation) {
			DestroyTextureImage(object);
		}
	}

	object->loaded = false;

	interfaceObjects.erase(
		InterfaceObjectDescriptor(object, object->depth));

	drawMutex.unlock();
	recreateMutex.unlock();
}

Model::InstanceDescriptor& Video::AddInstance(Model* model)
{
	drawMutex.lock();

	model->instances.add();

	Model::InstanceDescriptor& instance = model->instances.back();
	instance.active = false;
	instance.modelPosition = glm::mat4(1.0f);

	if (model->loaded) {
		recreateMutex.lock();

		CreateUniformBuffers(instance);
		CreateDescriptorPool(instance.descriptorPool);
		CreateDescriptorSets(model, instance);

		recreateMutex.unlock();
	}

	drawMutex.unlock();

	return instance;
}

void Video::RemoveInstance(Model* model, size_t index)
{
	auto it = model->instances.begin();

	for (size_t idx = 0; idx < index; ++idx) {
		++it;
	}

	Model::InstanceDescriptor& instance = *it;

	instance.active = false;

	bool ready;

	do {
		ready = true;

		instance.freeMutex.lock();

		for (bool val : instance.free) {
			ready = ready && val;
		}

		instance.freeMutex.unlock();
	} while (work && !ready);

	drawMutex.lock();

	if (model->loaded) {
		recreateMutex.lock();

		DestroyDescriptorPool(instance.descriptorPool);
		DestroyUniformBuffers(instance);

		recreateMutex.unlock();
	}

	model->instances.erase(it);

	drawMutex.unlock();
}

void Video::Start()
{
	work = true;
	MainLoop();
	work = false;
}

void Video::Stop()
{
	work = false;
}

void Video::SetCamera(
	const glm::vec3* position,
	const glm::vec3* target,
	const glm::vec3* up)
{
	cameraMutex.lock();

	if (position) {
		camera.position = *position;
	}

	if (target) {
		camera.target = *target;
	}

	if (up) {
		camera.up = *up;
	}

	cameraMutex.unlock();
}

void Video::CreateSkybox(std::string fileName)
{
	LoadSkybox(fileName);
}

void Video::BindKey(
	int key,
	int action,
	void* object,
	void (*callback)(int, int, void*))
{
	KeyBinding binding;

	binding.key = key;
	binding.action = action;
	binding.object = object;
	binding.callback = callback;

	keyBindMutex.lock();
	keyBindings.push_back(binding);
	keyBindMutex.unlock();
}

void Video::ClearKeyBindings()
{
	keyBindMutex.lock();
	keyBindings.clear();
	keyBindMutex.unlock();
}

void Video::SetCursorMoveCallback(
	void* object,
	void (*callback)(double, double, void*))
{
	cursorMoveController = object;
	cursorMoveCallback = callback;
}

void Video::SetMouseButtonCallback(
	void* object,
	void (*callback)(int, int, void*))
{
	mouseButtonController = object;
	mouseButtonCallback = callback;
}

void Video::SetNormalMouseMode()
{
	cameraCursor = false;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Video::SetCameraMouseMode()
{
	cameraCursor = true;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Video::SetScrollCallback(
	void* object,
	void (*callback)(double, double, void*))
{
	scrollController = object;
	scrollCallback = callback;
}

// GPUMemoryManager
GPUMemoryManager::GPUMemoryManager(
	VkDevice device,
	uint32_t partSize,
	uint32_t memoryTypeIndex,
	uint32_t alignment)
{
	this->alignment = alignment;
	this->device = device;
	this->partSize = partSize;
	this->memoryTypeIndex = memoryTypeIndex;

	AddPartition();
}

GPUMemoryManager::~GPUMemoryManager()
{
	for (const auto& memory : partitions) {
		vkFreeMemory(device, memory, nullptr);
	}
}

void GPUMemoryManager::AddPartition()
{
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = alignment * partSize;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory;

	VkResult res = vkAllocateMemory(device, &allocInfo, nullptr, &memory);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate memory");
	}

	partitions.push_back(memory);
	partitionData.push_back(std::vector<bool>(partSize, false));
}

GPUMemoryManager::MemoryAllocationProperties
GPUMemoryManager::Allocate(uint32_t size)
{
	uint32_t sizeInSectors = size / alignment + 1;

	uint32_t partitionNum = 0;
	uint32_t sectorNum = 0;

	bool goodPart = false;

	for (size_t partIdx = 0; partIdx < partitionData.size(); ++partIdx) {
		std::vector<bool>& partDesc = partitionData[partIdx];
		goodPart = false;

		size_t lim = partDesc.size() + 1 - sizeInSectors;

		for (size_t i = 0; i < lim; ++i) {
			goodPart = true;
			size_t j;

			for (j = 0; j < sizeInSectors;) {
				if (partDesc[i + j]) {
					goodPart = false;
					break;
				}

				++j;
			}

			if (goodPart) {
				partitionNum = partIdx;
				sectorNum = i;
				break;
			} else {
				i = i + j;
			}
		}

		if (goodPart) {
			break;
		}
	}

	if (!goodPart) {
		AddPartition();
		sectorNum = 0;
		partitionNum = partitions.size() - 1;
	}

	MemoryAllocationProperties props;
	props.size = size;
	props.offset = alignment * sectorNum;
	props.memory = partitions[partitionNum];

	for (size_t i = 0; i < sizeInSectors; ++i) {
		partitionData[partitionNum][i + sectorNum] = true;
	}

	return props;
}

void GPUMemoryManager::Free(
	GPUMemoryManager::MemoryAllocationProperties allocation)
{
	size_t partIdx = 0;

	for (const auto& memory : partitions) {
		if (memory == allocation.memory) {
			break;
		}

		++partIdx;
	}

	uint32_t sizeInSectors = allocation.size / alignment + 1;
	uint32_t sectorNum = allocation.offset / alignment;

	for (size_t i = 0; i < sizeInSectors; ++i) {
		partitionData[partIdx][i + sectorNum] = false;
	}
}

// Model
void Model::UpdateBuffers(
	const std::vector<Vertex>& vertices,
	const std::vector<VertexIndexType>& indices)
{
	if (loaded) {
		throw std::runtime_error("model is loaded");
	}

	this->vertices = vertices;
	this->indices = indices;
}

void Model::SetTextureName(const std::string& name)
{
	textureName = name;
}

size_t Model::GetInstanceCount()
{
	return instances.size();
}

Model::InstanceDescriptor& Model::GetInstance(size_t index)
{
	auto it = instances.begin();

	for (size_t idx = 0; idx < index; ++idx) {
		++it;
	}

	return *it;
}

// Model::Vertex
VkVertexInputBindingDescription Model::Vertex::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Model::Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
Model::Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	return attributeDescriptions;
}
