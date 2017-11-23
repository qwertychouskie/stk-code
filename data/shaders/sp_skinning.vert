#ifdef GL_ES
uniform sampler2D skinning_tex;
#else
uniform samplerBuffer skinning_tex;
#endif

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 4) in vec2 i_uv_two;
layout(location = 5) in vec4 i_tangent;
layout(location = 6) in ivec4 i_joint;
layout(location = 7) in vec4 i_weight;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 11) in vec4 i_misc_data;
layout(location = 12) in int i_skinning_offset;

#stk_include "utils/get_world_location.vert"

out vec3 tangent;
out vec3 bitangent;
out vec3 normal;
out vec2 uv;
out vec2 uv_two;
out vec4 color;
out float camdist;
flat out vec2 color_change;

void main(void)
{
    vec4 model_rotation = normalize(vec4(i_rotation.xyz, i_scale.w));
    vec4 idle_position = vec4(i_position, 1.0);
    vec4 idle_normal = i_normal;
    vec4 idle_tangent = vec4(i_tangent.xyz, 0.0);
    vec4 skinned_position = vec4(0.0);
    vec4 skinned_normal = vec4(0.0);
    vec4 skinned_tangent = vec4(0.0);

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
        skinned_normal += i_weight[i] * joint_matrix * idle_normal;
        skinned_tangent += i_weight[i] * joint_matrix * idle_tangent;
    }

    vec4 world_position = getWorldPosition(i_origin, model_rotation, i_scale.xyz,
        skinned_position.xyz);
    vec3 world_normal = rotateVector(model_rotation, skinned_normal.xyz);
    vec3 world_tangent = rotateVector(model_rotation, skinned_tangent.xyz);

    tangent = (u_view_matrix * vec4(world_tangent, 0.0)).xyz;
    bitangent = (u_view_matrix *
       // bitangent sign
      vec4(cross(world_normal, world_tangent) * i_tangent.w, 0.0)
      ).xyz;
    normal = (u_view_matrix * vec4(world_normal, 0.0)).xyz;

    uv = vec2(i_uv.x + i_misc_data.x, i_uv.y + i_misc_data.y);
    uv_two = i_uv_two;

    color = i_color.zyxw;
    camdist = length(u_view_matrix * world_position);
    color_change = i_misc_data.zw;
    gl_Position = u_projection_view_matrix * world_position;
}
