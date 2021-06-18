#ifndef VIDEO_H
#define VIDEO_H

#include <vector>
#include <string>
#include <optional>
#include <set>
#include <algorithm>
#include <array>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Video
{
public:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 3>
			GetAttributeDescriptions();
	};

private:
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

	struct UniformBufferObject
	{
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	const size_t MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

		{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	GLFWwindow* window;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<VkImage> swapchainImages;
	VkFormat swapchainImageFormat;
	VkExtent2D swapchainExtent;
	std::vector<VkImageView> swapchainImageViews;
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapchainFramebuffers;
	VkCommandPool commandPool;
	VkCommandPool transferCommandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	VkFence bufferCopyFence;
	size_t currentFrame;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	bool framebufferResized;

	bool enableValidationLayers;
	std::vector<const char*> validationLayers;

	std::vector<const char*> deviceExtensions;

	void InitWindow(int width, int height);
	void CloseWindow();

	void InitVulkan();
	void CloseVulkan();
	void PrepareSwapchain();
	void CleanupSwapchain();

	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapchain();
	void CreateImageViews();
	void DestroyImageViews();
	void CreateGraphicsPipeline();
	void DestroyGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();
	void DestroyFramebuffers();
	void CreateCommandPools();
	void DestroyCommandPools();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void DestroySyncObjects();
	void RecreateSwapchain();
	void CreateVertexBuffer();
	void DestroyVertexBuffer();
	void CreateIndexBuffer();
	void DestroyIndexBuffer();
	void CreateDescriptorSetLayout();
	void CreateUniformBuffers();
	void DestroyUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();
	void CreateDepthResources();
	void DestroyDepthResources();

	void CreateTextureImage();
	void DestroyTextureImage();
	void CreateTextureImageView();
	void DestroyTextureImageView();

	void CreateTextureSampler();
	void DestroyTextureSampler();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	static void FramebufferResizeCallback(
		GLFWwindow* window,
		int width,
		int height);

	VkShaderModule CreateShaderModule(uint32_t size, const uint32_t* code);
	void CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	void CreateImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory);

	bool IsDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckValidationLayerSupport(
		std::vector<const char*> validationLayers);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(
		const VkSurfaceCapabilitiesKHR& capabilities);
	uint32_t FindMemoryType(
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);
	void CopyBuffer(
		VkBuffer srcBuffer,
		VkBuffer dstBuffer,
		VkDeviceSize size);
	void UpdateUniformBuffer(uint32_t currentImage);
	void TransitionImageLayout(
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);
	void CopyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height);
	VkImageView CreateImageView(VkImage image, VkFormat format);

	void MainLoop();
	void DrawFrame();

public:
	Video(int width, int height, bool validate = false);
	~Video();
	void Start();
};

#endif
