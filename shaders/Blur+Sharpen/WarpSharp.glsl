// @gips_version=1 @coord=none

// "WarpSharp" filter, as described in
// https://www.virtualdub.org/blog2/entry_079.html

// This is a multi-pass filter that requires one "side channel" for internal
// data. Hence, it destroys the alpha channel ... sorry for that.

uniform float strength;
uniform float normThresh = 1.0;  // normalization threshold @max=1.4
uniform float blurEn = 1.0;      // @switch smooth gradient map

const vec3 rgb2gray = vec3(0.299, 0.587, 0.114);

// @filter=off
vec4 run_pass1(vec2 pos) {
    float tl = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, -1)).rgb, rgb2gray);
    float t  = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, -1)).rgb, rgb2gray);
    float tr = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, -1)).rgb, rgb2gray);
    float  l = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,  0)).rgb, rgb2gray);
    float  r = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(+1,  0)).rgb, rgb2gray);
    float bl = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, +1)).rgb, rgb2gray);
    float b  = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, +1)).rgb, rgb2gray);
    float br = dot(textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, +1)).rgb, rgb2gray);
    return vec4(textureLod(gips_tex, pos, 0.0).rgb, length(vec2(
        tr + br - tl - bl + 2.0 * (r - l),
        bl + br - tl - tr + 2.0 * (b - t)
    )));
}

// @filter=off
vec4 run_pass2(vec2 pos) {
    vec4 c = textureLod(gips_tex, pos, 0.0);
    if (blurEn < 0.5) { return c; }
    return vec4(c.rgb, 0.25 * c.a 
         + 0.125  * ( textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,  0)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2(+1,  0)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, -1)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, +1)).a)
         + 0.0625 * ( textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, -1)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, -1)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, +1)).a
                    + textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, +1)).a));
}

// @filter=on
vec4 run_pass3(vec2 pos) {
    float tl = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, -1)).a;
    float t  = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, -1)).a;
    float tr = textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, -1)).a;
    float  l = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,  0)).a;
    float  r = textureLodOffset(gips_tex, pos, 0.0, ivec2(+1,  0)).a;
    float bl = textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, +1)).a;
    float b  = textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, +1)).a;
    float br = textureLodOffset(gips_tex, pos, 0.0, ivec2(+1, +1)).a;
    vec2 delta = vec2(
        tr + br - tl - bl + 2.0 * (r - l),
        bl + br - tl - tr + 2.0 * (b - t)
    );
    float dlen = length(delta);
    float gain = 1.0 / max(dlen, 1.0/65536.0);
    if (dlen < normThresh) {
        gain = mix(1.0, gain, dlen / normThresh);
    }
    return vec4(textureLod(gips_tex, pos - (gain * strength) * (delta / gips_image_size), 0.0).rgb, 1.0);
}
