uniform int layer;

#ifdef GL_ES
uniform sampler2D skinning_tex;
#else
uniform samplerBuffer skinning_tex;
#endif

layout(location = 0) in vec3 i_position;
layout(location = 3) in vec2 i_uv;
layout(location = 6) in ivec4 i_joint;
layout(location = 7) in vec4 i_weight;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 12) in int i_skinning_offset;

#stk_include "utils/get_world_location.vert"

out vec2 uv;

void main(void)
{
    vec4 model_rotation = normalize(vec4(i_rotation.xyz, i_scale.w));
    vec4 idle_position = vec4(i_position, 1.0);
    vec4 skinned_position = vec4(0.0);

    for (int i = 0; i < 4; i++)
    {
#ifdef GL_ES
        mat4 joint_matrix = mat4(
            texelFetch(skinning_tex, ivec2
                (0, clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES)), 0),
            texelFetch(skinning_tex, ivec2
                (1, clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES)), 0),
            texelFetch(skinning_tex, ivec2
                (2, clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES)), 0),
            texelFetch(skinning_tex, ivec2
                (3, clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES)), 0));
#else
        mat4 joint_matrix = mat4(
            texelFetch(skinning_tex,
                clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES) * 4),
            texelFetch(skinning_tex,
                clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES) * 4 + 1),
            texelFetch(skinning_tex,
                clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES) * 4 + 2),
            texelFetch(skinning_tex,
                clamp(i_joint[i] + i_skinning_offset, 0, MAX_BONES) * 4 + 3));
#endif
        skinned_position += i_weight[i] * joint_matrix * idle_position;
    }

    vec4 world_position = getWorldPosition(i_origin, model_rotation, i_scale.xyz,
        skinned_position.xyz);
    uv = i_uv;
    gl_Position = ShadowViewProjMatrixes[layer] * world_position;
}
