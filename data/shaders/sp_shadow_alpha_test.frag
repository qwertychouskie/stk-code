uniform sampler2D layer_one_tex;

in vec2 uv;
out vec4 FragColor;

void main(void)
{
    vec4 col = texture(layer_one_tex, uv);
    if (col.a < 0.5)
    {
        discard;
    }
    FragColor = vec4(exp(32. * (2. * gl_FragCoord.z - 1.) / gl_FragCoord.w));
}
