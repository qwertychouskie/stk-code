// gloss map
uniform sampler2D tex_layer_2;
// normal mask
uniform sampler2D tex_layer_3;

in vec3 tangent;
in vec3 bitangent;
in vec3 normal;
in vec2 uv;
out vec3 o_encoded_normal;

#stk_include "utils/encode_normal.frag"

void main()
{
    vec3 tangent_space_normal = 2.0 * texture(tex_layer_3, uv).rgb - 1.0;
    float gloss = texture(tex_layer_2, uv).x;

    vec3 frag_tangent = normalize(tangent);
    vec3 frag_bitangent = normalize(bitangent);
    vec3 frag_normal = normalize(normal);
    mat3 t_b_n = mat3(frag_tangent, frag_bitangent, frag_normal);
    vec3 world_normal = t_b_n * tangent_space_normal;

    o_encoded_normal.xy = 0.5 * EncodeNormal(normalize(world_normal)) + 0.5;
    o_encoded_normal.z = gloss;
}
