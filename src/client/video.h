#ifndef VIDEO_H
#define VIDEO_H

#include <vector>
#include <string>

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Video
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	GLFWwindow* window;

	void InitWindow(int width, int height);
	void CloseWindow();
	void InitVulkan();
	void CloseVulkan();
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckValidationLayerSupport(
		std::vector<const char*> validationLayers);
	void MainLoop();
public:
	Video(int width, int height);
	~Video();
};

#endif
