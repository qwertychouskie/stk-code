uniform mat4 rsm_matrix;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec4 Texcoord;
layout(location = 8) in vec4 matrix_1;
layout(location = 9) in vec4 matrix_2;
layout(location = 10) in vec4 matrix_3;
layout(location = 11) in vec4 matrix_4;
layout(location = 12) in vec4 matrix_5;
layout(location = 13) in vec4 matrix_6;

out vec3 nor;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;

#stk_include "utils/getworldmatrix.vert"

void main(void)
{
    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    mat4 inverse_model_matrix = getInverseModelMatrix(matrix_4, matrix_5, matrix_6);
    mat4 model_view_projection_matrix = rsm_matrix * model_matrix;
    mat4 TransposeInverseModel = transpose(inverse_model_matrix);
    gl_Position = model_view_projection_matrix * vec4(Position, 1.);
    nor = (TransposeInverseModel * Normal).xyz;
    uv = Texcoord.xy;
    uv_bis = Texcoord.zw;
    color = Color.zyxw;
}
