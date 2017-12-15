uniform int layer;

layout(location = 0) in vec3 i_position;
layout(location = 1) in vec4 i_normal;
layout(location = 2) in vec4 i_color;
layout(location = 3) in vec2 i_uv;
layout(location = 4) in vec2 i_uv_two;
layout(location = 5) in vec4 i_tangent;
layout(location = 8) in vec3 i_origin;
layout(location = 9) in vec4 i_rotation;
layout(location = 10) in vec4 i_scale;
layout(location = 13) in uvec4 i_bindless_texture_0;
layout(location = 14) in uvec4 i_bindless_texture_1;
layout(location = 15) in uvec4 i_bindless_texture_2;

#stk_include "utils/get_world_location.vert"

out vec2 uv;


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

#ifdef VSLayer
    gl_Layer = layer;
#endif

    vec4 world_position = getWorldPosition(i_origin, i_rotation, i_scale.xyz,
        i_position);
    uv = i_uv;
    gl_Position = u_shadow_projection_view_matrices[layer] * world_position;
}
