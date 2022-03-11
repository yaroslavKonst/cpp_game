#include "validation_layers_helper.h"

std::vector<const char*> ValidationLayersHelper::GetLayers()
{
	if (!ValidationLayersActive) {
		return std::vector<const char*>();
	}

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
