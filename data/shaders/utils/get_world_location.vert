vec3 rotateVector(vec4 quat, vec3 vec)
{
    // Quaternion in 2_10_10_10, w is guaranteed to be positive
    return vec + 2.0 * cross(cross(vec, quat.xyz) +
        // calculate .w in quaternion
        (1 - sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z))
        * vec, quat.xyz);
}

vec4 getWorldPosition(vec3 origin, vec4 rotation, vec3 scale, vec3 local_pos)
{
    local_pos = local_pos * scale;
    local_pos = rotateVector(rotation, local_pos);
    local_pos = local_pos + origin;
    return vec4(local_pos, 1.0);
}
