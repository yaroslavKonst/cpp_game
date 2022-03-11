#include "instance_holder.h"

InstanceHolder::InstanceHolder(const std::string& name)
{
	_instanceCreated = false;
	_applicationName = name;
}

InstanceHolder::~InstanceHolder()
{
	Destroy();
}

void InstanceHolder::Destroy()
{
	if (_instanceCreated) {
		vkDestroyInstance(_instance, nullptr);
		_instanceCreated = false;
	}
}

void InstanceHolder::Create()
{
	if (_instanceCreated) {
		return;
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = _applicationName.c_str();
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
	instanceInfo.ppEnabledExtensions = glfwExtensions;

	std::vector<const char*> validation Layers =
		ValidationLayersHelper::GetLayers();

	instanceInfo.enabledLayerCount =
		static_cast<uint32_t>(validationLayers.size());

	if (instanceInfo.enabledLayerCount > 0) {
		instanceInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VkResult res = vkCreateInstance(&instanceInfo, nullptr, &_instance);

	if (res != VK_SUCCESS) {
		throw std::runtime_error("failed to create VkInstance");
	}

	_instanceCreated = true;
}

VkInstance* InstanceHolder::GetInstance()
{
	return &_instance;
}
