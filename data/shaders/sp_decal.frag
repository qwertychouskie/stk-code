uniform sampler2D layer_one_tex;
uniform sampler2D layer_two_tex;
uniform sampler2D gloss_map;

in vec2 uv;
in vec2 uv_bis;
out vec4 FragColor;

#stk_include "utils/getLightFactor.frag"

void main(void)
{
    vec4 color = texture(layer_one_tex, uv);
    vec4 layer_two_tex = texture(layer_two_tex, uv_bis);
    layer_two_tex.rgb = layer_two_tex.a * layer_two_tex.rgb;
    color.rgb = layer_two_tex.rgb + color.rgb * (1. - layer_two_tex.a);
    float specmap = texture(gloss_map, uv).g;
    FragColor = vec4(getLightFactor(color.xyz, vec3(1.), specmap, 0.), 1.);
}
