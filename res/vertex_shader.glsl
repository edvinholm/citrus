
#version 130

#define DEBUG_NORMALS 0

uniform mat4 projection;
uniform mat4 transform;

uniform vec4 color_multiplier;
uniform bool mode_2d;

in vec3 position;
in vec2 uvs;
in vec4 color;
in float texture;
in vec3 normal;

out vec3 vertex_position;
out vec2 fragment_texcoord;
out vec3 fragment_position;
out vec3 fragment_world_position;
out vec4 fragment_color;
out float fragment_texture;
out float fragment_light_factor;


const vec3 sun = vec3(-0.196, -0.588, 0.784);
const vec3 light_2d = vec3(0, 0, 1);

void main() {

    gl_Position = vec4(position, 1.0); // Do we need to set gl_Position????

    vec4 world_position = gl_Position * transform;
    gl_Position = world_position * projection;

    vec4 world_normal = normalize(vec4(normal, 0) * transform);

    fragment_color = color * color_multiplier;
    vec3 light_source = (mode_2d) ? light_2d : sun;
    float light_factor = (0.5f + min(0.5f, 0.65f * dot(world_normal.xyz, light_source)));
    fragment_color.xyz *= light_factor;

#if DEBUG_NORMALS
    if(!mode_2d) {
        fragment_color.xyz = world_normal.xyz;
        fragment_color.xy *= -1;
    
        fragment_color.xyz *= 0.5f;
        fragment_color.xyz += vec3(0.5f, 0.5f, 0.5f);
    }
#endif

    fragment_texcoord = uvs;
    vertex_position = gl_Position.xyz;
    fragment_position = gl_Position.xyz;
    fragment_world_position = world_position.xyz;

    fragment_texture = texture;
    fragment_light_factor = light_factor;
}

