
#version 130

//TODO @Speed: Combine matrices on CPU?
uniform mat4 projection;
uniform mat4 transform;
//--

// @ColorUniform: uniform vec4 color;

in vec3 aVertexPosition;
in vec2 aTexCoord;
in vec3 aNormal;
in vec4 color;

out vec3 vertex_position;
out vec2 fragment_texcoord;
out vec3 fragment_position;
out vec3 fragment_world_position;
out vec4 fragment_color;

void main() {

    fragment_color = color;

    vec4 normal;

    gl_Position = vec4(aVertexPosition, 1.0);
    normal = normalize(vec4(aNormal, 1.0));

    vec4 world_position = gl_Position * transform;
    gl_Position = world_position * projection;

    fragment_texcoord = aTexCoord;
    vertex_position = gl_Position.xyz;
    fragment_position = gl_Position.xyz;
    fragment_world_position = world_position.xyz;
}

