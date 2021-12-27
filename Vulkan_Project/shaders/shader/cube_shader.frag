#version 450
#extension GL_ARB_separate_shader_objects : enable


//#define

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in uint fragType;

layout(location = 0) out vec4 outColor;

#define EMPTY (0)
#define HOUSE (1)

#define STATE_HIDDEN (1u << 30)
#define STATE_SHOW (1u << 31)

void main()
{
    if(fragType == (HOUSE | STATE_SHOW))
    {
        //outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
        outColor = texture(texSampler, fragTexCoord);
        //outColor = vec4(fragColor, 1.0f);
    }
}