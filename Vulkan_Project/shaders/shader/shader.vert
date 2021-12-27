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
layout(location = 3) in vec4 InstanceMat0;
layout(location = 4) in vec4 InstanceMat1;
layout(location = 5) in vec4 InstanceMat2;
layout(location = 6) in vec4 InstanceMat3;
layout(location = 7) in uint InstanceType;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint fragType;


void main()
{
    gl_Position = pushConsts.ProjView * mat4(InstanceMat0, InstanceMat1, InstanceMat2, InstanceMat3) * vec4(inPosition, 1.0f);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragType = InstanceType;
}

//vec4(1.0f, 0.0f, 0.0f, 0.0f)
//vec4(0.0f, 1.0f, 0.0f, 0.0f)
//vec4(0.0f, 0.0f, 1.0f, 0.0f)
//vec4(0.0f, 0.0f, 0.0f, 1.0f)