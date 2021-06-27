#ifndef VIDEO_H
#define VIDEO_H

#include <vector>
#include <list>
#include <string>
#include <optional>
#include <set>
#include <algorithm>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Model;

class Video
{
public:
	//Subclasses
	class GPUMemoryManager
	{
	public:
		struct MemoryAllocationProperties
		{
			uint32_t size;
			uint32_t offset;
			VkDeviceMemory memory;
		};

	private:
		uint32_t partSize; // Partition size in KB.
		uint32_t memoryTypeIndex;
		uint32_t alignment;
		VkDevice device;
		std::vector<VkDeviceMemory> partitions;
		std::vector<std::vector<bool>> partitionData;

		void AddPartition();

	public:
		GPUMemoryManager(
			VkDevice device,
			uint32_t partSize,
			uint32_t memoryTypeIndex);
		~GPUMemoryManager();

		MemoryAllocationProperties Allocate(uint32_t size);
		void Free(MemoryAllocationProperties properties);
	};

private:
	// Types
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;
	};

	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	enum GPUMemoryManagerType {
		MANAGER_VERTEX,
		MANAGER_INDEX,
		MANAGER_UNIFORM,
		MANAGER_TEXTURE
	};

	// Global objects
	std::string ApplicationName;
	std::vector<const char*> deviceExtensions;

	bool validate;
	std::vector<const char*> validationLayers;

	std::set<Model*> models;

	uint32_t maxFramesInFlight;
	uint32_t currentFrame;

	bool framebufferResized;

	GPUMemoryManager* vertexBufferMemoryManager;
	GPUMemoryManager* indexBufferMemoryManager;
	GPUMemoryManager* textureImageMemoryManager;
	GPUMemoryManager* uniformBufferMemoryManager;

	void InitVulkan();
	void CloseVulkan();

	VkInstance instance;
	void CreateInstance();
	void DestroyInstance();

	VkSurfaceKHR surface;
	void CreateSurface();
	void DestroySurface();

	VkPhysicalDevice physicalDevice;
	void PickPhysicalDevice();

	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	void CreateLogicalDevice();
	void DestroyLogicalDevice();

	VkCommandPool commandPool;
	VkCommandPool transferCommandPool;
	void CreateCommandPools();
	void DestroyCommandPools();

	VkFence bufferCopyFence;
	void CreateSyncObjects();
	void DestroySyncObjects();

	VkDescriptorSetLayout descriptorSetLayout;
	void CreateDescriptorSetLayout();
	void DestroyDescriptorSetLayout();

	VkSampler textureSampler;
	void CreateTextureSampler();
	void DestroyTextureSampler();

	// Swapchain objects
	void CreateSwapchain();
	void DestroySwapchain();
	void RecreateSwapchain();

	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	void CreateSwapchainInstance();
	void DestroySwapchainInstance();

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	void CreateDepthResources();
	void DestroyDepthResources();

	std::vector<VkImageView> swapchainImageViews;
	void CreateImageViews();
	void DestroyImageViews();

	bool allowDescriptorPoolCreation;
	void CreateDescriptorPools();
	void DestroyDescriptorPools();
	void CreateDescriptorPool(VkDescriptorPool& descriptorPool);
	void DestroyDescriptorPool(VkDescriptorPool& descriptorPool);

	bool allowDescriptorSetCreation;
	void CreateDescriptorSets();
	void DestroyDescriptorSets();
	void CreateDescriptorSets(Model* model);
	void DestroyDescriptorSets(Model* model);

	bool allowUniformBufferCreation;
	void CreateUniformBuffers();
	void DestroyUniformBuffers();
	void CreateUniformBuffers(Model* model);
	void DestroyUniformBuffers(Model* model);

	bool allowVertexBufferCreation;
	void CreateVertexBuffers();
	void DestroyVertexBuffers();
	void CreateVertexBuffer(Model* model);
	void DestroyVertexBuffer(Model* model);

	bool allowIndexBufferCreation;
	void CreateIndexBuffers();
	void DestroyIndexBuffers();
	void CreateIndexBuffer(Model* model);
	void DestroyIndexBuffer(Model* model);

	bool allowTextureImageCreation;
	void CreateTextureImages();
	void DestroyTextureImages();
	void CreateTextureImage(Model* model);
	void DestroyTextureImage(Model* model);

	VkRenderPass renderPass;
	void CreateRenderPass();
	void DestroyRenderPass();

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	void CreateGraphicsPipeline();
	void DestroyGraphicsPipeline();

	std::vector<VkFramebuffer> swapchainFramebuffers;
	void CreateFramebuffers();
	void DestroyFramebuffers();

	std::vector<VkCommandBuffer> commandBuffers;
	void CreateCommandBuffers();
	void DestroyCommandBuffers();

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	void CreateSwapchainSyncObjects();
	void DestroySwapchainSyncObjects();

	// GLFW objects
	GLFWwindow* window;

	void InitWindow(int width, int height);
	void CloseWindow();

	// Helper functions
	std::vector<const char*> GetValidationLayers();
	static void FramebufferResizeCallback(
		GLFWwindow* window,
		int width,
		int height);
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapchainSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapchainPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapchainExtent(
		const VkSurfaceCapabilitiesKHR& capabilities);
	void CreateImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory);
	uint32_t FindMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat();
	VkFormat FindSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);
	VkImageView CreateImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags);
	void TransitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	bool HasStencilComponent(VkFormat format);
	void CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		GPUMemoryManagerType managerType,
		VkBuffer& buffer,
		GPUMemoryManager::MemoryAllocationProperties& memoryAllocation);
	void CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	void CopyBuffer(
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size);
	void CreateImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		GPUMemoryManagerType managerType,
		VkImage& image,
		GPUMemoryManager::MemoryAllocationProperties& memoryAllocation);
	void CopyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height);
	VkShaderModule CreateShaderModule(uint32_t size, const uint32_t* code);
	void CreateCommandBuffer(uint32_t imageIndex);
	void UpdateUniformBuffers(uint32_t imageIndex);

	// Rendering
	void DrawFrame();
	void MainLoop();

public:
	Video(const std::string& name, int width, int height);
	~Video();

	Model* LoadModel(std::string fileName);
	void DestroyModel(Model* model);

	void Start();
	void Stop();
};

class Model
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::vector<VkVertexInputAttributeDescription>
			GetAttributeDescriptions();
	};

public:
	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

private:
	friend class Video;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	std::string textureName;

	VkBuffer vertexBuffer;
	Video::GPUMemoryManager::MemoryAllocationProperties vertexBufferMemory;

	VkBuffer indexBuffer;
	Video::GPUMemoryManager::MemoryAllocationProperties indexBufferMemory;

public:
	UniformBufferObject ubo;

private:
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	VkImage textureImage;
	VkImageView textureImageView;
	Video::GPUMemoryManager::MemoryAllocationProperties textureImageMemory;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<Video::GPUMemoryManager::MemoryAllocationProperties>
		uniformBufferMemory;

	Model()
	{ }

	~Model()
	{ }
};

#endif
