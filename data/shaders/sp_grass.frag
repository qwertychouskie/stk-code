// spm layer 1 texture
uniform sampler2D tex_layer_0;
// gloss map
uniform sampler2D tex_layer_2;
// colorization mask
uniform sampler2D tex_layer_4;

flat in vec2 color_change;

in vec3 normal;
in vec2 uv;

layout(location = 0) out vec3 o_normal_depth;
layout(location = 1) out vec3 o_gloss_map;
layout(location = 2) out vec3 o_diffuse_color;

#stk_include "utils/encode_normal.frag"
#stk_include "utils/rgb_conversion.frag"

void main(void)
{
    vec4 col = texture(tex_layer_0, uv);
    if (col.a < 0.5)
    {
        discard;
    }

    if (color_change.x > 0.0)
    {
        float mask = texture(tex_layer_4, uv).a;
        vec3 old_hsv = rgbToHsv(col.rgb);
        float mask_step = step(mask, 0.5);
        vec2 new_xy = mix(vec2(old_hsv.x, old_hsv.y), vec2(color_change.x,
            max(old_hsv.y, color_change.y)), vec2(mask_step, mask_step));
        vec3 new_color = hsvToRgb(vec3(new_xy.x, new_xy.y, old_hsv.z));
        col = vec4(new_color.r, new_color.g, new_color.b, col.a);
    }

	o_normal_depth.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
	o_normal_depth.z = texture(tex_layer_2, uv).x;
    o_gloss_map = 0.1 * texture(tex_layer_2, uv).rgb;
    o_diffuse_color = col.xyz;
}
