layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 4) in vec2 i_uv_two;
layout(location = 5) in vec4 i_tangent;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 11) in vec4 i_misc_data;

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
    //vec4 quat_rot = normalize(vec4(i_rotation.xyz, sqrt(1.0 - (i_rotation.x * i_rotation.x + i_rotation.y * i_rotation.y + i_rotation.z * i_rotation.z))));
    vec4 quat_rot = normalize(vec4(i_rotation.xyz, i_scale.w));
    vec4 world_position = getWorldPosition(i_origin, quat_rot, i_scale.xyz,
        i_position);
    vec3 world_normal = rotateVector(quat_rot, i_normal.xyz);
    vec3 world_tangent = rotateVector(quat_rot, i_tangent.xyz);

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
