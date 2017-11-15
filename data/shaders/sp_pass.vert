uniform vec2 texture_trans;

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec4 Texcoord;
layout(location = 4) in vec4 Tangent;
layout(location = 5) in vec4 Bitangent;
layout(location = 8) in vec4 matrix_1;
layout(location = 9) in vec4 matrix_2;
layout(location = 10) in vec4 matrix_3;
layout(location = 11) in vec4 matrix_4;
layout(location = 12) in vec4 matrix_5;
layout(location = 13) in vec4 matrix_6;
layout(location = 14) in vec4 misc_data;

#stk_include "utils/getworldmatrix.vert"

out vec3 nor;
out vec3 tangent;
out vec3 bitangent;
out vec2 uv;
out vec2 uv_bis;
out vec4 color;
out float camdist;
flat out vec4 color_data;

void main(void)
{
    color = Color.zyxw;
    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    mat4 inverse_model_matrix = getInverseModelMatrix(matrix_4, matrix_5, matrix_6);
    mat4 ModelViewProjectionMatrix = ProjectionViewMatrix * model_matrix;
    mat4 TransposeInverseModelView = transpose(inverse_model_matrix * InverseViewMatrix);
    gl_Position = ModelViewProjectionMatrix * vec4(Position, 1.);
    // Keep orthogonality
    nor = (TransposeInverseModelView * Normal).xyz;
    // Keep direction
    tangent = (ViewMatrix * model_matrix * Tangent).xyz;
    bitangent = (ViewMatrix * model_matrix * Bitangent).xyz;
    uv = vec2(Texcoord.x + texture_trans.x, Texcoord.y + texture_trans.y);
    uv_bis = Texcoord.zw;
    color_data = misc_data;
    camdist = length(ViewMatrix * model_matrix * vec4(Position, 1.));
}
