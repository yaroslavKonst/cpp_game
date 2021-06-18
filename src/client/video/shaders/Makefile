.PHONY: shaders

shaders: vert.spv frag.spv

vert.spv: shader.vert
	glslangValidator --target-env vulkan1.0 --vn VertexShader shader.vert

frag.spv: shader.frag
	glslangValidator --target-env vulkan1.0 --vn FragmentShader shader.frag
