// @gips_version=1 @coord=rel @filter=off

uniform float r1         = 0.75;  // inner radius @min=0 @max=5
uniform float r2         = 1.00;  // outer radius @min=0 @max=5
uniform float rotation   = 0.00;  // @angle
uniform float saturation = 1.00;
uniform float lightness  = 1.00;
uniform float gradient   = 0.00;  // @toggle draw saturation gradient
uniform float oklch      = 0.00;  // @toggle OkLch (max. saturation = 0.37!)
uniform float gamma      = 2.20;  // OkLch output gamma @min=0.5 @max=5.0

const float TAU = 6.283185307179586;

vec4 run(vec2 pos) {
    float ri = min(r1, r2), ro = max(r1, r2);
    float h = fract(1.5 - (atan(pos.x, pos.y) + rotation) / TAU);
    float r = length(pos);
    float alpha = ro - r;
    float s = saturation;
    float l = lightness;
    if (gradient < 0.5) {
        alpha = min(alpha, r - ri);
    } else {
        l *= min(r / max(ri, 0.01), 1.0);
        s *= clamp((ro - r) / max(ro - ri, 0.01), 0.0, 1.0);
    }
    vec3 c;
    if (oklch < 0.5) {
        c = abs(fract(vec3(h) + vec3(1.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0);
        c = l * mix(vec3(1.0), clamp(c - vec3(1.0), 0.0, 1.0), s);
    } else {
        vec3 lms = vec3(l)
                 + vec3(+0.3963377774, -0.1055613458, -0.0894841775) * s * cos(h * TAU)
                 + vec3(+0.2158037573, -0.0638541728, -1.2914855480) * s * sin(h * TAU);
        lms = lms * lms * lms;
        c = pow(vec3(
            dot(lms, vec3(+4.0767416621, -3.3077115913, +0.2309699292)),
            dot(lms, vec3(-1.2684380046, +2.6097574011, -0.3413193965)),
            dot(lms, vec3(-0.0041960863, -0.7034186147, +1.7076147010))
        ), vec3(1.0 / gamma));
    }
    return vec4(c, alpha / fwidth(r));
}
