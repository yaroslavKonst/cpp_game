#!/bin/bash

glslangValidator --target-env vulkan1.0 --vn VertexShader shader.vert
glslangValidator --target-env vulkan1.0 --vn FragmentShader shader.frag
