uniform vec3 wind_direction;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 11) in vec4 i_misc_data;

#stk_include "utils/get_world_location.vert"

out vec3 normal;
out vec2 uv;
flat out vec2 color_change;

void main()
{
    vec3 test = sin(wind_direction * (i_position.y * 0.1));
    test += cos(wind_direction) * 0.7;

    vec4 model_rotation = normalize(vec4(i_rotation.xyz, i_scale.w));
    vec4 world_position = getWorldPosition(i_origin + test * i_color.r,
        model_rotation, i_scale.xyz, i_position);
    vec3 world_normal = rotateVector(model_rotation, i_normal.xyz);

    normal = (u_view_matrix * vec4(world_normal, 0.0)).xyz;
    uv = i_uv;
    color_change = i_misc_data.zw;
    gl_Position = u_projection_view_matrix * world_position;
}
