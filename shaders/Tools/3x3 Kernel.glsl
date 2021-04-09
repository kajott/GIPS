// @gips_version=1 @coord=none @filter=off

uniform vec3 row0 = vec3(0.0, 0.0, 0.0);  // @min=-64 @max=64 abc
uniform vec3 row1 = vec3(0.0, 1.0, 0.0);  // @min=-64 @max=64 def
uniform vec3 row2 = vec3(0.0, 0.0, 0.0);  // @min=-64 @max=64 ghi
uniform float div = 1.0;                  // @min=0 @max=256 divisor
uniform float autodiv = 1.0;              // @switch determine divisor automatically
uniform float offset;

vec4 run(vec2 pos) {
    vec4 center = textureLod(gips_tex, pos, 0.0);
    float d = (autodiv < 0.5) ? div
            : (dot(row0, vec3(1.0)) + dot(row1, vec3(1.0)) + dot(row2, vec3(1.0)));
    if (abs(d) < 1E-6) { d = 1.0; }
    vec3 res = row0.x * textureLodOffset(gips_tex, pos, 0.0, ivec2(-1,-1)).rgb
             + row0.y * textureLodOffset(gips_tex, pos, 0.0, ivec2( 0,-1)).rgb
             + row0.z * textureLodOffset(gips_tex, pos, 0.0, ivec2( 1,-1)).rgb
             + row1.x * textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 0)).rgb
             + row1.y * center.rgb
             + row1.z * textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 0)).rgb
             + row2.x * textureLodOffset(gips_tex, pos, 0.0, ivec2(-1, 1)).rgb
             + row2.y * textureLodOffset(gips_tex, pos, 0.0, ivec2( 0, 1)).rgb
             + row2.z * textureLodOffset(gips_tex, pos, 0.0, ivec2( 1, 1)).rgb;
    return vec4(res / d + offset, center.a);
}
