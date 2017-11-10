// from Crytek "a bit more deferred CryEngine"
vec2 EncodeNormal(vec3 normal)
{
     // Project normal positive hemisphere to unit circle
    // We project from point (0,0,-1) to the plane [0,(0,0,-1)]
    // den = dot (l.d, p.n)
    // t = -(dot (p.n, l.p) + p.d) / den
    vec2 p = normal.xy / (abs (normal.z) + 1.0);

    // Convert unit circle to square
    // We add epsilon to avoid division by zero
    float d = abs (p.x) + abs (p.y) + 0.0001;
    float r = length (p);
    vec2 q = p * r / d;

    // Mirror triangles to outer edge if z is negative
    float z_is_negative = max (-sign (normal.z), 0.0);
    vec2 q_sign = sign (q);
    q_sign = sign (q_sign + vec2 (0.5, 0.5));
    // Reflection
    // qr = q - 2 * n * (dot (q, n) - d) / dot (n, n)
    q -= z_is_negative * (dot (q, q_sign) - 1.0) * q_sign;

    return q;
}