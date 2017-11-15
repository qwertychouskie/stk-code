uniform sampler2D layer_one_tex;

in vec2 uv;
in vec4 color;
out vec4 FragColor;

void main(void)
{
    vec4 col = texture(layer_one_tex, uv);
    if (col.a < 0.5) discard;
    col.xyz *= color.xyz;
    FragColor = vec4(col.xyz, 1.);
}
