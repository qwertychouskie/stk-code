
vec3 DecodeNormal(vec2 encodedNormal)
{
    vec2 p = encodedNormal;
   
    // Find z sign
    float zsign = sign (1.0 - abs (p.x) - abs (p.y));
    // Map outer triangles to center if encoded z is negative
    float z_is_negative = max (-zsign, 0.0);
    vec2 p_sign = sign (p);
    p_sign = sign (p_sign + vec2 (0.5, 0.5));
    // Reflection
    // qr = q - 2 * n * (dot (q, n) - d) / dot (n, n)
    p -= z_is_negative * (dot (p, p_sign) - 1.0) * p_sign;

    // Convert square to unit circle
    // We add epsilon to avoid division by zero
    float r = abs (p.x) + abs (p.y);
    float d = length (p) + 0.00001;
    vec2 q = p * r / d;

    // Deproject unit circle to sphere
    float den = 2.0 / (dot (q, q) + 1.0);
    vec3 v = vec3(den * q, zsign * (den - 1.0));

    return v;
}