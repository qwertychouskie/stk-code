vec3 rotateVector(vec4 quat, vec3 vec)
{
    return vec + 2.0 * cross(cross(vec, quat.xyz) + quat.w * vec, quat.xyz);
}

vec4 getWorldPosition(vec3 origin, vec4 rotation, vec3 scale, vec3 local_pos)
{
    local_pos = local_pos * scale;
    local_pos = rotateVector(rotation, local_pos);
    local_pos = local_pos + origin;
    return vec4(local_pos, 1.0);
}
