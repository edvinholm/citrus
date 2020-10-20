
#version 130

#ifdef GL_ES
precision highp float;
#endif

in vec3 vertex_position;
in vec2 fragment_texcoord;
in vec3 fragment_position;
in vec3 fragment_world_position;
in vec4 fragment_color;

uniform sampler2D texture_;
uniform bool texture_present;

out vec4 fragment_color_out;

void main() {

    if(texture_present)
    {
        fragment_color_out = texture2D(texture_, fragment_texcoord);   
        fragment_color_out *= fragment_color;

        //fragment_color_out *= vec4(1 - fragment_color_out.r, 1 - fragment_color_out.g, 1 - fragment_color_out.b, 1);

        //   fragment_color_out = vec4(fragment_texcoord.x, fragment_texcoord.y, 0, 1);
        
    }
    else
    {
        fragment_color_out = fragment_color;
    }
    
}
