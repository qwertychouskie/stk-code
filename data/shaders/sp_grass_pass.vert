uniform vec3 wind_direction;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 11) in vec4 i_misc_data;

#if defined(Use_Bindless_Texture)
layout(location = 13) in uvec4 i_bindless_texture_0;
layout(location = 14) in uvec4 i_bindless_texture_1;
layout(location = 15) in uvec4 i_bindless_texture_2;
#elif defined(Use_Array_Texture)
layout(location = 13) in uvec4 i_array_texture_0;
layout(location = 14) in uvec2 i_array_texture_1;
#endif

#stk_include "utils/get_world_location.vert"

out vec3 normal;
out vec2 uv;
flat out float hue_change;

#if defined(Use_Bindless_Texture)
flat out sampler2D tex_layer_0;
flat out sampler2D tex_layer_1;
flat out sampler2D tex_layer_2;
flat out sampler2D tex_layer_3;
flat out sampler2D tex_layer_4;
flat out sampler2D tex_layer_5;
#elif defined(Use_Array_Texture)
flat out float array_0;
flat out float array_1;
flat out float array_2;
flat out float array_3;
flat out float array_4;
flat out float array_5;
#endif

void main()
{
#if defined(Use_Bindless_Texture)
    tex_layer_0 = sampler2D(i_bindless_texture_0.xy);
    tex_layer_1 = sampler2D(i_bindless_texture_0.zw);
    tex_layer_2 = sampler2D(i_bindless_texture_1.xy);
    tex_layer_3 = sampler2D(i_bindless_texture_1.zw);
    tex_layer_4 = sampler2D(i_bindless_texture_2.xy);
    tex_layer_5 = sampler2D(i_bindless_texture_2.zw);
#elif defined(Use_Array_Texture)
    array_0 = float(i_array_texture_0.x);
    array_1 = float(i_array_texture_0.y);
    array_2 = float(i_array_texture_0.z);
    array_3 = float(i_array_texture_0.w);
    array_4 = float(i_array_texture_1.x);
    array_5 = float(i_array_texture_1.y);
#endif

    vec3 test = sin(wind_direction * (i_position.y * 0.1));
    test += cos(wind_direction) * 0.7;

    vec4 world_position = getWorldPosition(i_origin + test * i_color.r,
        i_rotation, i_scale.xyz, i_position);
    vec3 world_normal = rotateVector(i_rotation, i_normal.xyz);

    normal = (u_view_matrix * vec4(world_normal, 0.0)).xyz;
    uv = i_uv;
    hue_change = i_misc_data.z;
    gl_Position = u_projection_view_matrix * world_position;
}
