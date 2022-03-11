#ifndef INSTANCE_HOLDER_H
#define INSTANCE_HOLDER_H

#include <vulkan/vulkan.h>

class InstanceHolder
{
public:
	InstanceHolder(const std::string& name);
	~InstanceHolder();

	void Create();
	void Destroy();

	VkInstance* GetInstance();

private:
	VkInstance _instance;
	bool _instanceCreated;

	std::string _applicationName;
};

#endif
