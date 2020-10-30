
#version 130

#ifdef GL_ES
precision highp float;
#endif

in vec3 vertex_position;
in vec2 fragment_texcoord;
in vec3 fragment_position;
in vec3 fragment_world_position;
in vec4 fragment_color;
in float fragment_texture; // 0 is no texture

uniform sampler2D texture_1;
uniform sampler2D texture_2;
uniform sampler2D texture_3;
uniform sampler2D texture_4;

out vec4 fragment_color_out;


void main() {

    fragment_color_out = fragment_color;

    // @Speed
    // NOTE: Someone said this can be slow on older versions of OpenGL.
    if(fragment_texture >= 0.5f) {
        if(fragment_texture < 2.5f) {
            if(fragment_texture < 1.5f) { // 1
                fragment_color_out *= texture2D(texture_1, fragment_texcoord);
            } else { // 2
                fragment_color_out *= texture2D(texture_2, fragment_texcoord);
            }
        }
        else {
            if(fragment_texture < 3.5f) { // 3
                fragment_color_out *= texture2D(texture_3, fragment_texcoord);
            } else { // 4
                fragment_color_out *= texture2D(texture_4, fragment_texcoord);
            }
        }
    }
    
}
