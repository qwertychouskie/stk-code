uniform sampler2D layer_one_tex;

in vec2 uv;
in vec3 nor;
in vec4 color;

layout (location = 0) out vec3 rsm_color;
layout (location = 1) out vec3 rsm_normal;

void main()
{
    vec4 col = texture(layer_one_tex, uv);
    if (col.a < .5)
    {
        discard;
    }
    rsm_color = col.xyz * color.rgb;
    rsm_normal = .5 * normalize(nor) + .5;
}
