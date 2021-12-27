#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint fragType;

layout(location = 0) out vec4 outColor;

#define EMPTY (0)
#define HOUSE (1)
#define COUNT (2)

void main()
{
    if(fragType == HOUSE)
    {
        //outColor = vec4(fragColor * texture(texSampler, fragTexCoord * 2.0).rgb, 1.0);
        outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
        //outColor = texture(texSampler, fragTexCoord * 2.0);
    }
}