// @gips_version=1 @coord=rel @filter=on

uniform vec2 centerI;  // @min=-2 @max=2  input center
uniform vec2 centerO;  // @min=-2 @max=2 output center
uniform float scale;   // @min=-3 @max=3 scale (logarithmic)
uniform float rot;     // @angle @max=360 output rotation
uniform float axes;    // @int @min=0 @max=16 mirror axes
uniform float angle;   // @angle @max=360 axis angle
uniform vec4 bg;       // @color background color

vec2 mirror(vec2 p, float a) {
    float c = cos(a);
    float s = sin(a);
    p *= mat2(c, s, -s, c);
    p.x = abs(p.x);
    p *= mat2(c, -s, s, c);
    return p;
}

vec4 run(vec2 pos) {
    // transform and sample foreground
    float s = exp2(-scale);
    float c = s * cos(rot);
          s = s * sin(rot);
    pos = (pos - centerO) * mat2(c, s, -s, c);
    for (float ax = 0.5;  ax < axes;  ax += 1.0) {
        pos = mirror(pos, angle + (ax - 0.5) * 3.14159 / axes);
    }
    vec4 fg = pixel(pos + centerI);

    // clip to border
    float outside = max(
        abs(pos.x) - max(1.0, gips_image_size.x / gips_image_size.y),
        abs(pos.y) - max(1.0, gips_image_size.y / gips_image_size.x)
    );
    fg.a *= 1.0 - smoothstep(0, fwidth(outside), outside);

    // compose final color
    float bga = bg.a * (1.0 - fg.a);
    fg.rgb = fg.rgb * fg.a + bg.rgb * bga;
    fg.a += bga;
    return vec4(fg.rgb / fg.a, fg.a);
}
