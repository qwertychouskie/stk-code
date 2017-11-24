uniform sampler2D layer_one_tex;
uniform int fog_enabled;
uniform float custom_alpha;

in vec2 uv;
in vec4 color;
out vec4 FragColor;

void main()
{
    vec4 diffusecolor = texture(layer_one_tex, uv);
    vec4 finalcolor = vec4(0.);
    if (fog_enabled == 0)
    {
        finalcolor = diffusecolor;
        finalcolor.xyz *= color.xyz;
        finalcolor.a *= color.a;
    }
    else
    {
        diffusecolor.xyz *= color.xyz;
        diffusecolor.a *= color.a;
        vec3 tmp = vec3(gl_FragCoord.xy / u_screen, gl_FragCoord.z);
        tmp = 2. * tmp - 1.;
        vec4 xpos = vec4(tmp, 1.0);
        xpos = u_inverse_projection_matrix * xpos;
        xpos.xyz /= xpos.w;
        float dist = length(xpos.xyz);
        float fog = smoothstep(fog_data.x, fog_data.y, dist);
        fog = min(fog, fog_data.z);
        finalcolor = fog_color * fog + diffusecolor * (1. - fog);
    }
    FragColor = vec4(finalcolor.rgb * (finalcolor.a * (1.0 - custom_alpha)),
        finalcolor.a * (1.0 - custom_alpha));
}
