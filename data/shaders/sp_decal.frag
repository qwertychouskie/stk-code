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

layout(location = 0) out vec3 o_normal_depth;
layout(location = 1) out vec3 o_gloss_map;
layout(location = 2) out vec3 o_diffuse_color;

#stk_include "utils/encode_normal.frag"

void main(void)
{
	o_normal_depth.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
	o_normal_depth.z = texture(tex_layer_2, uv).x;

    o_gloss_map = texture(tex_layer_2, uv).rgb;

    vec4 color = texture(tex_layer_0, uv);
    vec4 layer_two_tex = texture(tex_layer_1, uv_two);
    layer_two_tex.rgb = layer_two_tex.a * layer_two_tex.rgb;
    o_diffuse_color = layer_two_tex.rgb + color.rgb * (1.0 - layer_two_tex.a);
}

