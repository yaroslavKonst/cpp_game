.PHONY: shaders

shaders: object_vert.spv object_frag.spv interface_vert.spv interface_frag.spv

object_vert.spv: shader.vert
	glslangValidator --target-env vulkan1.0 --vn ObjectVertexShader -o object_vert.spv shader.vert

object_frag.spv: shader.frag
	glslangValidator --target-env vulkan1.0 --vn ObjectFragmentShader -o object_frag.spv shader.frag

interface_vert.spv: interface.vert
	glslangValidator --target-env vulkan1.0 --vn InterfaceVertexShader -o interface_vert.spv interface.vert

interface_frag.spv: interface.frag
	glslangValidator --target-env vulkan1.0 --vn InterfaceFragmentShader -o interface_frag.spv interface.frag
