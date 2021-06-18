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
#include <glm/glm.hpp>

class Video
{
public:
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription GetBindingDescription();
		static std::array<VkVertexInputAttributeDescription, 2>
			GetAttributeDescriptions();
	};

private:
	const size_t MAX_FRAMES_IN_FLIGHT = 2;

	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	};

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

	bool framebufferResized;

	bool enableValidationLayers;
	std::vector<const char*> validationLayers;

	std::vector<const char*> deviceExtensions;

	void InitWindow(int width, int height);
	void CloseWindow();

	void InitVulkan();
	void CloseVulkan();

	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapchain();
	void CreateImageViews();
	void DestroyImageViews();
	void CreateGraphicsPipeline();
	void CreateRenderPass();
	void CreateFramebuffers();
	void DestroyFramebuffers();
	void CreateCommandPools();
	void DestroyCommandPools();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void DestroySyncObjects();
	void RecreateSwapchain();
	void CleanupSwapchain();
	void CreateVertexBuffer();
	void DestroyVertexBuffer();

	static void FramebufferResizeCallback(
		GLFWwindow* window,
		int width,
		int height);

	VkShaderModule CreateShaderModule(uint32_t size, const uint32_t* code);

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

	void MainLoop();
	void DrawFrame();

public:
	Video(int width, int height, bool validate = false);
	~Video();
	void Start();
};

#endif
