uniform int layer;
uniform int skinning_offset;
uniform int joint_count;
#ifndef GL_ES
uniform samplerBuffer skinning_tbo;
#endif

layout(location = 0) in vec3 Position;
layout(location = 3) in vec4 Texcoord;
layout(location = 6) in ivec4 Joint;
layout(location = 7) in vec4 Weight;
layout(location = 8) in vec4 matrix_1;
layout(location = 9) in vec4 matrix_2;
layout(location = 10) in vec4 matrix_3;

#ifdef VSLayer
out vec2 uv;
#else
out vec2 tc;
out int layer_id;
#endif

#stk_include "utils/getworldmatrix.vert"

void main(void)
{

#ifdef VSLayer
    gl_Layer = layer;
    uv = Texcoord.xy;
#else
    layer_id = layer;
    tc = Texcoord.xy;
#endif

    mat4 model_matrix = getModelMatrix(matrix_1, matrix_2, matrix_3);
    vec4 skinned_position = vec4(0.);
    if (Weight[0] < 0.01)
    {
        skinned_position = vec4(Position, 1.);
    }
    else
    {
        for (int i = 0; i < 4; i++)
        {
            if (Weight[i] < 0.01)
            {
                break;
            }
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
        }
#endif
        }
    }
    gl_Position = ShadowViewProjMatrixes[layer] * model_matrix * skinned_position;
}
