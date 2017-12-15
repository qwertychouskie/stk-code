uniform vec3 wind_direction;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 11) in vec4 i_misc_data;
layout(location = 13) in uvec4 i_bindless_texture_0;
layout(location = 14) in uvec4 i_bindless_texture_1;
layout(location = 15) in uvec4 i_bindless_texture_2;

#stk_include "utils/get_world_location.vert"

out vec3 normal;
out vec2 uv;
flat out vec2 color_change;

#ifdef Use_Bindless_Texture
flat out sampler2D tex_layer_0;
flat out sampler2D tex_layer_1;
flat out sampler2D tex_layer_2;
flat out sampler2D tex_layer_3;
flat out sampler2D tex_layer_4;
flat out sampler2D tex_layer_5;
#endif

void main()
{
#ifdef Use_Bindless_Texture
    tex_layer_0 = sampler2D(i_bindless_texture_0.xy);
    tex_layer_1 = sampler2D(i_bindless_texture_0.zw);
    tex_layer_2 = sampler2D(i_bindless_texture_1.xy);
    tex_layer_3 = sampler2D(i_bindless_texture_1.zw);
    tex_layer_4 = sampler2D(i_bindless_texture_2.xy);
    tex_layer_5 = sampler2D(i_bindless_texture_2.zw);
#endif

    vec3 test = sin(wind_direction * (i_position.y * 0.1));
    test += cos(wind_direction) * 0.7;

    vec4 world_position = getWorldPosition(i_origin + test * i_color.r,
        i_rotation, i_scale.xyz, i_position);
    vec3 world_normal = rotateVector(i_rotation, i_normal.xyz);

    normal = (u_view_matrix * vec4(world_normal, 0.0)).xyz;
    uv = i_uv;
    color_change = i_misc_data.zw;
    gl_Position = u_projection_view_matrix * world_position;
}
