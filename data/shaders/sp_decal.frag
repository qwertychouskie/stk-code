// spm layer 1 texture
uniform sampler2D tex_layer_0;
// spm layer 2 texture
uniform sampler2D tex_layer_1;
// gloss map
uniform sampler2D tex_layer_2;

in vec2 uv;
in vec2 uv_two;
out vec4 o_frag_color;

#stk_include "utils/getLightFactor.frag"

void main(void)
{
    vec4 color = texture(tex_layer_0, uv);
    vec4 layer_two_tex = texture(tex_layer_1, uv_two);
    layer_two_tex.rgb = layer_two_tex.a * layer_two_tex.rgb;
    color.rgb = layer_two_tex.rgb + color.rgb * (1.0 - layer_two_tex.a);
    float specmap = texture(tex_layer_2, uv).g;
    o_frag_color = vec4(getLightFactor(color.xyz, vec3(1.), specmap, 0.0),
        1.0);
}
