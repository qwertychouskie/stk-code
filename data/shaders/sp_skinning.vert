uniform vec2 texture_trans;
uniform int skinning_offset;
uniform int joint_count;
#ifndef GL_ES
uniform samplerBuffer skinning_tbo;
#endif

layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Normal;
layout(location = 2) in vec4 Color;
layout(location = 3) in vec4 Texcoord;
layout(location = 4) in vec4 Tangent;
layout(location = 5) in vec4 Bitangent;
layout(location = 6) in ivec4 Joint;
layout(location = 7) in vec4 Weight;
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
flat out vec4 color_data;

void main(void)
{
    color = Color.zyxw;
    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    mat4 inverse_model_matrix = getInverseModelMatrix(matrix_4, matrix_5, matrix_6);
    mat4 ModelViewProjectionMatrix = ProjectionViewMatrix * model_matrix;
    mat4 TransposeInverseModelView = transpose(inverse_model_matrix * InverseViewMatrix);
    vec4 skinned_position = vec4(0.);
    vec4 skinned_normal = vec4(0.);
    vec4 skinned_tangent = vec4(0.);
    vec4 skinned_bitangent = vec4(0.);
    if (Weight[0] < 0.01)
    {
        skinned_position = vec4(Position, 1.);
        skinned_normal = Normal;
        skinned_tangent = Tangent;
        skinned_bitangent = Bitangent;
    }
    else
    {
#ifdef GL_ES
        mat4 skin_matrix =
            Weight[0] * mat4(
                joint_matrices[Joint[0] * 4 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[0] * 4 + 1 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[0] * 4 + 2 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[0] * 4 + 3 +
                (skinning_offset + joint_count * gl_InstanceID * 4)]) +
            Weight[1] * mat4(
                joint_matrices[Joint[1] * 4 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[1] * 4 + 1 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[1] * 4 + 2 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[1] * 4 + 3 +
                (skinning_offset + joint_count * gl_InstanceID * 4)]) +
            Weight[2] * mat4(
                joint_matrices[Joint[2] * 4 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[2] * 4 + 1 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[2] * 4 + 2 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[2] * 4 + 3 +
                (skinning_offset + joint_count * gl_InstanceID * 4)]) +
            Weight[3] * mat4(
                joint_matrices[Joint[3] * 4 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[3] * 4 + 1 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[3] * 4 + 2 +
                (skinning_offset + joint_count * gl_InstanceID * 4)],
                joint_matrices[Joint[3] * 4 + 3 +
                (skinning_offset + joint_count * gl_InstanceID * 4)]);
        skinned_position = skin_matrix * vec4(Position, 1.);
        skinned_normal = skin_matrix * Normal;
        skinned_tangent = skin_matrix * Tangent;
        skinned_bitangent = skin_matrix * Bitangent;
#else
        for (int i = 0; i < 4; i++)
        {
            if (Weight[i] < 0.01)
            {
                break;
            }
            mat4 joint_matrix = mat4(
                texelFetch(skinning_tbo, Joint[i] * 4 +
                (skinning_offset + joint_count * gl_InstanceID * 4)),
                texelFetch(skinning_tbo, Joint[i] * 4 + 1 +
                (skinning_offset + joint_count * gl_InstanceID * 4)),
                texelFetch(skinning_tbo, Joint[i] * 4 + 2 +
                (skinning_offset + joint_count * gl_InstanceID * 4)),
                texelFetch(skinning_tbo, Joint[i] * 4 + 3 +
                (skinning_offset + joint_count * gl_InstanceID * 4)));
            skinned_position += Weight[i] * joint_matrix * vec4(Position, 1.);
            skinned_normal += Weight[i] * joint_matrix * Normal;
            skinned_tangent += Weight[i] * joint_matrix * Tangent;
            skinned_bitangent += Weight[i] * joint_matrix * Bitangent;
        }
#endif
    }
    gl_Position = ModelViewProjectionMatrix * skinned_position;
    // Keep orthogonality
    nor = (TransposeInverseModelView * skinned_normal).xyz;
    // Keep direction
    tangent = (ViewMatrix * model_matrix * skinned_tangent).xyz;
    bitangent = (ViewMatrix * model_matrix * skinned_bitangent).xyz;
    uv = vec2(Texcoord.x + texture_trans.x, Texcoord.y + texture_trans.y);
    uv_bis = Texcoord.zw;
    color_data = misc_data;
}
