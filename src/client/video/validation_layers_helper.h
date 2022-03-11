#ifndef VALIDATION_LAYERS_HELPER_H
#define VALIDATION_LAYERS_HELPER_H

#include "includes.h"

#include <vector>

class ValidationLayersHelper
{
	const bool ValidationLayersActive = true;

public:
	static std::vector<const char*> GetLayers();
};

#endif
