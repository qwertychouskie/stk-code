// spm layer 1 texture
uniform sampler2D tex_layer_0;

in vec2 uv;
out vec4 o_frag_color;

void main(void)
{
    vec4 col = texture(tex_layer_0, uv);
    if (col.a < 0.5)
    {
        discard;
    }
    o_frag_color = vec4(exp(32. * (2. * gl_FragCoord.z - 1.) / gl_FragCoord.w));
}
