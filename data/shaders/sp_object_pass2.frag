// spm layer 1 texture
uniform sampler2D tex_layer_0;
// gloss map
uniform sampler2D tex_layer_2;
// colorization mask
uniform sampler2D tex_layer_4;

flat in vec2 color_change;

in vec2 uv;
in vec4 color;
out vec4 o_frag_color;

#stk_include "utils/getLightFactor.frag"
#stk_include "utils/rgb_conversion.frag"

void main(void)
{
    vec4 col = texture(tex_layer_0, uv);

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

    col.xyz *= color.xyz;
    float specmap = texture(tex_layer_2, uv).g;
    float emitmap = texture(tex_layer_2, uv).b;
    o_frag_color = vec4(getLightFactor(col.xyz, vec3(1.), specmap, emitmap), 1.);
}
