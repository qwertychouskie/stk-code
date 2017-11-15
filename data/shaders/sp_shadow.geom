layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

in vec2 tc[3];
in int layer_one[3];
in int layer_two[3];
in int layer_three[3];
in int layer_four[3];

out vec2 uv;

void main(void)
{
    if (layer_one[0] != -1)
    {
        gl_Layer = layer_one[0];
        for (int i = 0; i < 3; i++)
        {
            uv = tc[i];
            gl_Position = ShadowViewProjMatrixes[layer_one[0]] *
                gl_in[i].gl_Position;
            EmitVertex();
        }
    }
    if (layer_two[0] != -1)
    {
        gl_Layer = layer_two[0];
        for (int i = 0; i < 3; i++)
        {
            uv = tc[i];
            gl_Position = ShadowViewProjMatrixes[layer_two[0]] *
                gl_in[i].gl_Position;
            EmitVertex();
        }
    }
    if (layer_three[0] != -1)
    {
        gl_Layer = layer_three[0];
        for (int i = 0; i < 3; i++)
        {
            uv = tc[i];
            gl_Position = ShadowViewProjMatrixes[layer_three[0]] *
                gl_in[i].gl_Position;
            EmitVertex();
        }
    }
    if (layer_four[0] != -1)
    {
        gl_Layer = layer_four[0];
        for (int i = 0; i < 3; i++)
        {
            uv = tc[i];
            gl_Position = ShadowViewProjMatrixes[layer_four[0]] *
                gl_in[i].gl_Position;
            EmitVertex();
        }
    }
    EndPrimitive();
}
