#ifndef VIDEO_H
#define VIDEO_H

#include <vector>
#include <string>
#include <optional>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

class Video
{
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
	};

	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	GLFWwindow* window;
	VkSurfaceKHR surface;

	bool enableValidationLayers;
	std::vector<const char*> validationLayers;

	void InitWindow(int width, int height);
	void CloseWindow();
	void InitVulkan();
	void CloseVulkan();
	void CreateInstance();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckValidationLayerSupport(
		std::vector<const char*> validationLayers);
	void MainLoop();
public:
	Video(int width, int height, bool validate = false);
	~Video();
};

#endif
