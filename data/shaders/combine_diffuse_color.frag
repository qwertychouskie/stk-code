uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D ssao_tex;
uniform sampler2D gloss_map;
uniform sampler2D diffuse_color;

out vec4 o_final_color;
#stk_include "utils/getPosFromUVDepth.frag"

void main()
{
    vec2 tc = gl_FragCoord.xy / u_screen;
    vec3 diffuseMatColor = texture(diffuse_color, tc).xyz;

    // Gloss map here is stored in red and green for spec and emit map
    // Real gloss channel is stored in normal and depth framebuffer .z
    float specMapValue = texture(gloss_map, tc).x;
    float emitMapValue = texture(gloss_map, tc).y;

    float ao = texture(ssao_tex, tc).x;
    vec3 DiffuseComponent = texture(diffuse_map, tc).xyz;
    vec3 SpecularComponent = texture(specular_map, tc).xyz;
    vec3 tmp = diffuseMatColor * DiffuseComponent * (1. - specMapValue) + SpecularComponent * specMapValue;
    vec3 emitCol = diffuseMatColor.xyz * diffuseMatColor.xyz * diffuseMatColor.xyz * 15.;
    o_final_color = vec4(tmp * ao + (emitMapValue * emitCol), 1.0);
}
