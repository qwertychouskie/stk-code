// spm layer 1 texture
uniform sampler2D tex_layer_0;

in vec2 uv;
in vec4 color;
out vec4 o_frag_color;

void main(void)
{
    vec4 col = texture(tex_layer_0, uv);
    if (col.a < 0.5)
    {
        discard;
    }
    col.xyz *= color.xyz;
    o_frag_color = vec4(col.xyz, 1.);
}
