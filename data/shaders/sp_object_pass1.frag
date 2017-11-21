uniform sampler2D tex_layer_2;

in vec3 normal;
in vec2 uv;
out vec3 EncodedNormal;

#stk_include "utils/encode_normal.frag"

void main(void)
{
	float gloss = texture(tex_layer_2, uv).x;
	EncodedNormal.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
	EncodedNormal.z = gloss;
}
