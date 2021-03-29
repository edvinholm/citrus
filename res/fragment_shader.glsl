
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
in float fragment_light_factor;

uniform sampler2D textures[4];

uniform vec3 lightbox_center;
uniform vec3 lightbox_radiuses;
uniform vec4 lightbox_color;

uniform bool do_edge_detection;


out vec4 fragment_color_out;


const float blur_kernel[] = float[9](
    1/9f, 1/9f, 1/9f,
    1/9f, 1/9f, 1/9f,
    1/9f, 1/9f, 1/9f);

const float edge_kernel[] = float[9](
    -1, -1, -1,
    -1,  8, -1,
    -1, -1, -1);

const float pixel_size = 1/2048f; // @Hardcoded

const float sample_offsets[] = float[9*2](
    -pixel_size, -pixel_size,   0, -pixel_size,    pixel_size, -pixel_size,
    -pixel_size, 0,             0, 0,              pixel_size, 0,
    -pixel_size, pixel_size,    0, pixel_size,     pixel_size, pixel_size);

#if 0

#define Get_Texture_Samples(Texture)            \
    if(do_edge_detection) {                     \
         \ 
        vec2 uv0 = fragment_texcoord - vec2(pixel_size, pixel_size); \
        vec2 uv = uv0; \
        \
        texture_samples[0] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[1] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[2] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        \
        uv.x = uv0.x;\
        \
        texture_samples[3] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[4] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[5] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        \
        uv.x = uv0.x;\
        \
        texture_samples[6] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[7] = texture2D(Texture, uv);\
        uv.x += pixel_size;\
        texture_samples[8] = texture2D(Texture, uv);        \
                                                    \
    } else {                                                            \
        fragment_color_out *= texture2D(Texture, fragment_texcoord);    \
    }                                                                   \

#endif

void main() {

    fragment_color_out = fragment_color;

    vec4 texture_samples[9];
    
    // @Speed
    // NOTE: Someone said this can be slow on older versions of OpenGL.
    if(fragment_texture >= 0.5f) {

        int texture_index;
        
        if(fragment_texture < 2.5f) {
            if(fragment_texture < 1.5f) {
                texture_index = 0;
            } else {
                texture_index = 1;
            }
        }
        else {
            if(fragment_texture < 3.5f) {
                texture_index = 2;
            } else {
                texture_index = 3;
            }
        }

        if(do_edge_detection) {
            texture_samples[0] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[0], sample_offsets[1]));
            texture_samples[1] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[2], sample_offsets[3]));
            texture_samples[2] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[4], sample_offsets[5]));
                                           
            texture_samples[3] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[6], sample_offsets[7]));
            texture_samples[4] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[8], sample_offsets[9]));
            texture_samples[5] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[10], sample_offsets[11]));
                                           
            texture_samples[6] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[12], sample_offsets[13]));
            texture_samples[7] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[14], sample_offsets[15]));
            texture_samples[8] = texture2D(textures[texture_index], fragment_texcoord + vec2(sample_offsets[16], sample_offsets[17]));

            vec4 edge = vec4(0, 0, 0, 0);
            edge += edge_kernel[0] * texture_samples[0];
            edge += edge_kernel[1] * texture_samples[1];
            edge += edge_kernel[2] * texture_samples[2];
            edge += edge_kernel[3] * texture_samples[3];
            edge += edge_kernel[4] * texture_samples[4];
            edge += edge_kernel[5] * texture_samples[5];
            edge += edge_kernel[6] * texture_samples[6];
            edge += edge_kernel[7] * texture_samples[7];
            edge += edge_kernel[8] * texture_samples[8];

            edge.a *= 0.5f;
            float edge_f = max(edge.r, max(edge.g, max(edge.b, edge.a)))/9.0f;

            fragment_color_out *= texture2D(textures[texture_index], fragment_texcoord);
            float multiplier = (1.0f - edge_f);
            fragment_color_out.rgb *= multiplier * multiplier * multiplier * multiplier;
            
        } else {
            fragment_color_out *= texture2D(textures[texture_index], fragment_texcoord);
        }
        
    }
    
    vec3 distance_to_lightbox_center = fragment_world_position.xyz - lightbox_center;
    if(abs(distance_to_lightbox_center.x) <= lightbox_radiuses.x &&
       abs(distance_to_lightbox_center.y) <= lightbox_radiuses.y &&
       abs(distance_to_lightbox_center.z) <= lightbox_radiuses.z)
    {
        // Inside lightbox
        float t = clamp(lightbox_color.w * fragment_light_factor, 0, 1);
        vec3  a = vec3(1,1,1) * (1 + clamp(fragment_light_factor, -1, 0));
        vec3  b = lightbox_color.xyz;
        fragment_color_out.xyz *= (a*(1-t) + b*t);
    }
    
}
