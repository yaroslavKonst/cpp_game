#ifndef VIDEO_H
#define VIDEO_H

#include <vector>
#include <string>
#include <optional>
#include <set>
#include <algorithm>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

class Video
{
	const int MAX_FRAMES_IN_FLIGHT = 2;

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
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	size_t currentFrame;

	uint32_t width;
	uint32_t height;

	bool enableValidationLayers;
	std::vector<const char*> validationLayers;

	std::vector<const char*> deviceExtensions;

	void InitWindow();
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
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSyncObjects();
	void DestroySyncObjects();

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

	void MainLoop();
	void DrawFrame();
public:
	Video(int w, int h, bool validate = false);
	~Video();
	void Start();
};

#endif
