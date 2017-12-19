#ifdef Use_Bindless_Texture
flat in sampler2D tex_layer_0;
flat in sampler2D tex_layer_2;
flat in sampler2D tex_layer_4;
#else
// spm layer 1 texture
uniform sampler2D tex_layer_0;
// gloss map
uniform sampler2D tex_layer_2;
#endif

flat in vec2 color_change;

in vec4 color;
in vec3 normal;
in vec2 uv;

layout(location = 0) out vec4 o_diffuse_color;
layout(location = 1) out vec3 o_normal_depth;
layout(location = 2) out vec2 o_gloss_map;

#stk_include "utils/encode_normal.frag"
#stk_include "utils/rgb_conversion.frag"

void main(void)
{
    vec4 col = texture(tex_layer_0, uv);

    if (color_change.x > 0.0)
    {
        float mask = col.a;
        vec3 old_hsv = rgbToHsv(col.rgb);
        float mask_step = step(mask, 0.5);
#if !defined(Advanced_Lighting_Enabled)
        float saturation = mask * 2.1;
#else
        float saturation = mask * 2.5;
#endif
        vec2 new_xy = mix(vec2(old_hsv.x, old_hsv.y), vec2(color_change.x,
            max(old_hsv.y, saturation)), vec2(mask_step, mask_step));
        vec3 new_color = hsvToRgb(vec3(new_xy.x, new_xy.y, old_hsv.z));
        col = vec4(new_color.r, new_color.g, new_color.b, 1.0);
    }

    vec3 final_color = col.xyz * color.xyz;
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
