#pragma tangram: extensions

#ifdef TANGRAM_SDF_MULTISAMPLING
    #ifdef GL_OES_standard_derivatives
        #extension GL_OES_standard_derivatives : enable
    #else
        #undef TANGRAM_SDF_MULTISAMPLING
    #endif
#endif

#ifdef GL_ES
    precision mediump float;
    #define LOWP lowp
#else
    #define LOWP
#endif

#pragma tangram: defines

const float emSize = 15.0 / 16.0;

uniform sampler2D u_tex0;
uniform sampler2D u_tex1;
uniform sampler2D u_tex2;
uniform sampler2D u_tex3;
uniform vec3 u_map_position;
uniform vec3 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform int u_pass;

#pragma tangram: uniforms

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_sdf_threshold;
varying float v_alpha;
varying float v_textureUnit;

#pragma tangram: global

float contour(in float d, in float w, float t) {
    return smoothstep(t - w, t + w, d);
}

float sample(sampler2D tex, in vec2 uv, float w, float t) {
    return contour(texture2D(tex, uv).a, w, t);
}

float sampleAlpha(sampler2D tex, in vec2 uv, float distance, float threshold) {
    const float smoothing = 0.0625 * emSize; // 0.0625 = 1.0/1em ratio
    float alpha = contour(distance, smoothing, threshold);

#ifdef TANGRAM_SDF_MULTISAMPLING
    const float aaSmooth = smoothing / 2.0;
    float dscale = 0.354; // 1 / sqrt(2)
    vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
    vec4 box = vec4(uv - duv, uv + duv);

    float asum = sample(tex, box.xy, aaSmooth, threshold)
               + sample(tex, box.zw, aaSmooth, threshold)
               + sample(tex, box.xw, aaSmooth, threshold)
               + sample(tex, box.zy, aaSmooth, threshold);

    alpha = mix(alpha, asum, 0.25);
#endif

    return alpha;
}

void main(void) {

    vec4 color = v_color;

    //float distance = texture2D(u_tex, v_texcoords).a;
    // color *= v_alpha * pow(sampleAlpha(tex, v_texcoords, distance, v_sdf_threshold), 0.4545);

    float dist;

    if (v_textureUnit == 0.0) {
        dist = texture2D(u_tex0, v_texcoords).a;
    } else if (v_textureUnit == 1.0) {
        dist = texture2D(u_tex1, v_texcoords).a;
    } else if (v_textureUnit == 2.0) {
        dist = texture2D(u_tex2, v_texcoords).a;
    } else if (v_textureUnit == 3.0) {
        dist = texture2D(u_tex3, v_texcoords).a;
    }

    // emSize 15/16 = .937
    // float s = 0.0625 * emSize; // 0.0625 = 1.0/1em ratio
    // // ==> .0666
    float s = 0.066;
    s *= 2.5;

    float alpha = smoothstep(v_sdf_threshold - s,
                             v_sdf_threshold + s,
                             dist);

    color *= v_alpha * alpha;

    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}
