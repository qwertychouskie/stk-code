uniform sampler2D layer_one_tex;
uniform sampler2D gloss_map;
uniform sampler2D colorization_mask;

in vec2 uv;
flat in vec4 color_data;
out vec4 FragColor;

#stk_include "utils/getLightFactor.frag"
#stk_include "utils/rgb_conversion.frag"

void main(void)
{
    vec4 color = texture(layer_one_tex, uv);
    if (color.a < 0.5) discard;

    if (color_data.w > 0.0)
    {
        float mask = texture(colorization_mask, uv).a;
        vec3 old_hsv = rgbToHsv(color.rgb);
        float mask_step = step(mask, 0.5);
        vec2 new_xy = mix(vec2(old_hsv.x, old_hsv.y), vec2(color_data.w, max(old_hsv.y, color_data.z)), vec2(mask_step, mask_step));
        color.xyz = hsvToRgb(vec3(new_xy.x, new_xy.y, old_hsv.z));
    }

    float specmap = texture(gloss_map, uv).g;
    float emitmap = texture(gloss_map, uv).b;
    vec3 LightFactor = color.xyz * 0.1 + getLightFactor(color.xyz, vec3(1.), specmap, emitmap);
    FragColor = vec4(LightFactor, 1.);
}
