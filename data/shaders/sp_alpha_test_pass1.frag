// spm layer 1 texture
uniform sampler2D tex_layer_0;
// gloss map
uniform sampler2D tex_layer_2;

in vec3 normal;
in vec2 uv;
out vec3 o_encoded_normal;

#stk_include "utils/encode_normal.frag"

void main()
{
    vec4 col = texture(tex_layer_0, uv);
    if (col.a < 0.5)
    {
        discard;
    }
    float gloss = texture(tex_layer_2, uv).x;
    o_encoded_normal.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
    o_encoded_normal.z = gloss;
}
