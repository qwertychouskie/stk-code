uniform int layer;
uniform vec3 windDir;

layout(location = 0) in vec3 Position;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec4 Texcoord;
layout(location = 8) in vec4 matrix_1;
layout(location = 9) in vec4 matrix_2;
layout(location = 10) in vec4 matrix_3;

#stk_include "utils/getworldmatrix.vert"

#ifdef VSLayer
out vec2 uv;
#else
out vec2 tc;
out int layer_id;
#endif

void main(void)
{

#ifdef VSLayer
    gl_Layer = layer;
    uv = Texcoord.xy;
#else
    layer_id = layer;
    tc = Texcoord.xy;
#endif

    vec3 test = sin(windDir * (Position.y* 0.5)) * 0.5;
    test += cos(windDir) * 0.7;
    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    gl_Position = ShadowViewProjMatrixes[layer] * model_matrix * vec4(Position + test * Color.r, 1.);
}
