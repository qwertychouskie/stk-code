// spm layer 1 texture
uniform sampler2D tex_layer_0;
// spm layer 2 texture
uniform sampler2D tex_layer_1;
// gloss map
uniform sampler2D tex_layer_2;
// colorization mask
uniform sampler2D tex_layer_4;

in vec3 normal;
in vec2 uv;
in vec2 uv_two;

layout(location = 0) out vec4 o_diffuse_color;
layout(location = 1) out vec3 o_normal_depth;
layout(location = 2) out vec2 o_gloss_map;

#stk_include "utils/encode_normal.frag"

void main(void)
{
    vec4 color = texture(tex_layer_0, uv);
    vec4 layer_two_tex = texture(tex_layer_1, uv_two);
    layer_two_tex.rgb = layer_two_tex.a * layer_two_tex.rgb;

    vec3 final_color = layer_two_tex.rgb + color.rgb * (1.0 - layer_two_tex.a);
#if !defined(Advanced_Lighting_Enabled)
#if !defined(sRGB_Framebuffer_Usable)
    final_color = final_color * 0.73; // 0.5 ^ (1. / 2.2)
#else
    final_color = final_color * 0.5;
#endif
#endif
    o_diffuse_color = vec4(final_color, 1.0);

#if defined(Advanced_Lighting_Enabled)
	o_normal_depth.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
	o_normal_depth.z = texture(tex_layer_2, uv).r;

    o_gloss_map = texture(tex_layer_2, uv).gb;
#endif
}

