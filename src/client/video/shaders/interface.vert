#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform Area {
	float x0;
	float y0;
	float x1;
	float y1;
} area;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

vec2 positions[6];

vec2 texCoords[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main()
{
	positions[0] = vec2(area.x0, area.y0);
	positions[1] = vec2(area.x0, area.y1);
	positions[2] = vec2(area.x1, area.y0);
	positions[3] = vec2(area.x1, area.y0);
	positions[4] = vec2(area.x0, area.y1);
	positions[5] = vec2(area.x1, area.y1);

	gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
	fragColor = vec3(1.0, 1.0, 1.0);
	fragTexCoord = texCoords[gl_VertexIndex];
}
