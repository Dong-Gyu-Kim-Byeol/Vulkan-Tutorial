#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 0) uniform UniformBufferObject
{
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PushConsts 
{
	mat4 ProjView;
} pushConsts;

// Vertex
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

// Instance
layout(location = 3) in uint InstanceType;
layout(location = 4) in vec3 InstancePos;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint fragType;

#define STATE_HIDDEN (1u << 30)
#define STATE_SHOW (1u << 31)

void main()
{
    if((InstanceType & STATE_SHOW) == STATE_SHOW)
    {
        gl_Position = pushConsts.ProjView * vec4(inPosition + InstancePos, 1.0f);
        fragColor = inColor;
        fragTexCoord = inTexCoord;
        fragType = InstanceType;
    }
}