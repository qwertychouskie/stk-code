uniform vec3 windDir;
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
layout(location = 14) in vec4 misc_data;

#stk_include "utils/getworldmatrix.vert"

out vec3 nor;
out vec2 uv;
flat out vec4 color_data;

void main()
{
    vec3 test = sin(windDir * (Position.y* 0.5)) * 0.5;
    test += cos(windDir) * 0.7;
    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    mat4 inverse_model_matrix = getInverseModelMatrix(matrix_4, matrix_5, matrix_6);
    mat4 new_model_matrix = model_matrix;
    mat4 new_inverse_model_matrix = inverse_model_matrix;
    new_model_matrix[3].xyz += test * Color.r;
    new_inverse_model_matrix[3].xyz -= test * Color.r;
    mat4 ModelViewProjectionMatrix = ProjectionViewMatrix * new_model_matrix;
    mat4 TransposeInverseModelView = transpose(InverseViewMatrix * new_inverse_model_matrix);
    gl_Position = ModelViewProjectionMatrix * vec4(Position, 1.);
    nor = (TransposeInverseModelView * Normal).xyz;
    uv = Texcoord.xy;
    color_data = misc_data;
}
