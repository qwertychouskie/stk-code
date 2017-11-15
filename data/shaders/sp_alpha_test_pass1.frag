uniform sampler2D layer_one_tex;
uniform sampler2D gloss_map;

in vec3 nor;
in vec2 uv;
out vec3 EncodedNormal;

#stk_include "utils/encode_normal.frag"

void main()
{
    vec4 col = texture(layer_one_tex, uv);
    if (col.a < 0.5)
        discard;
    float gloss = texture(gloss_map, uv).x;
    EncodedNormal.xy = 0.5 * EncodeNormal(normalize(nor)) + 0.5;
    EncodedNormal.z = gloss;
}
