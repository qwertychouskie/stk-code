// spm layer 1 texture
uniform sampler2D tex_layer_0;
// gloss map
uniform sampler2D tex_layer_2;

in vec4 color;
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

	o_normal_depth.xy = 0.5 * EncodeNormal(normalize(normal)) + 0.5;
	o_normal_depth.z = texture(tex_layer_2, uv).x;
    o_gloss_map = vec3(0.0);
    o_diffuse_color = col.xyz * color.xyz;
}

