// @gips_version=1 @coords=rel @filter=on

uniform float zoom;           // @min=-5 @max=5   camera zoom (logarithmic)
uniform vec2 camPos;          // @min=-2 @max=2   camera offset
uniform vec3 rotation;        // @min=-90 @max=90 plane rotation
uniform vec4 background;      // @color           background color
uniform float zAlpha;         // @switch          map depth to alpha
uniform float zKey;           // @min=-3 @max=3   depth offset
uniform float zRange = 1.0;   // @min=0 @max=3    depth range
uniform float zInvert;        // @switch @on=1 @off=0 invert depth-alpha

vec4 run(vec2 pos) {
    // matrix for the plane's internal coordinate system
    mat3 tbn = mat3(1., 0., 0.,
                    0., 1., 0.,
                    0., 0.,-1.);

    // rotate the plane
    vec3 a = radians(rotation);
    float s, c;
    s = sin(a.y), c = cos(a.y);
    tbn = tbn * mat3( c,0., s,
                     0.,1.,0.,
                     -s,0., c);
    s = sin(a.x), c = cos(a.x);
    tbn = tbn * mat3(1.,0.,0.,
                     0., c,-s,
                     0., s, c);
    s = sin(a.z), c = cos(a.z);
    tbn = tbn * mat3( c,-s,0., 
                      s, c,0.,
                     0.,0.,1.);

    // ray-plane intersection
    vec3 o = vec3(camPos, -1.0);
    vec3 d = normalize(vec3(pos, 1.0));
    vec3 hit = o - d * dot(o, tbn[2]) / dot(tbn[2], d);

    // get on-plane coordinates
    vec2 uv = vec2(dot(hit, tbn[0]), dot(hit, tbn[1])) * exp(-zoom);

    // clip against boundaries
    float outside = max(
        abs(uv.x) - max(1.0, gips_image_size.x / gips_image_size.y),
        abs(uv.y) - max(1.0, gips_image_size.y / gips_image_size.x)
    );
    outside = smoothstep(0, fwidth(outside), outside);

    // compute base color
    vec4 color = pixel(uv);
    color = vec4(mix(color.rgb, mix(background.rgb, color.rgb, (1.0 - background.a) * (1.0 - outside)), outside), mix(color.a, background.a, outside));

    // depth mapping
    if (zAlpha > 0.5) {
        color.a *= abs(zInvert - smoothstep(0.0, 1.0, abs(hit.z - zKey) / zRange));
    }

    return color;
}
