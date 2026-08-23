#version 330 core

in float vIntensity;
in float vRange;
in float vAge;

out vec4 FragColor;

uniform float maxRange;
uniform int   colorMode;    // 0 = Range, 1 = Intensity
uniform float historyDecay;
uniform bool  circularPts;  // discard corners to make circular points

// Turbo colormap approximation
vec3 colormapTurbo(float x) {
    float r = 0.137932 + x * ( 3.586736 + x * (-7.367018 + x * ( 6.162791 + x * -1.536647)));
    float g = 0.096738 + x * ( 2.158739 + x * (-1.464670 + x * ( 0.103099 + x * -0.063231)));
    float b = 0.231920 + x * (-1.011681 + x * ( 5.433290 + x * (-7.644498 + x *  3.181827)));
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

void main() {
    // Circular point discard
    if (circularPts) {
        vec2 coord = gl_PointCoord - vec2(0.5);
        float dist2 = dot(coord, coord);
        if (dist2 > 0.25) discard;
        // Soft anti-aliased edge
        float alpha_edge = 1.0 - smoothstep(0.20, 0.25, dist2);
        
        float t = (colorMode == 0) ? clamp(vRange / maxRange, 0.0, 1.0)
                                   : clamp(vIntensity / 255.0, 0.0, 1.0);
        vec3 color = colormapTurbo(t);
        if (vAge > 0.01) {
            color = mix(color, vec3(0.2, 0.4, 0.9), clamp(vAge * historyDecay, 0.0, 0.8));
        }
        float alpha = clamp(1.0 - (vAge * historyDecay), 0.15, 1.0) * alpha_edge;
        FragColor = vec4(color, alpha);
    } else {
        float t = (colorMode == 0) ? clamp(vRange / maxRange, 0.0, 1.0)
                                   : clamp(vIntensity / 255.0, 0.0, 1.0);
        vec3 color = colormapTurbo(t);
        if (vAge > 0.01) {
            color = mix(color, vec3(0.2, 0.4, 0.9), clamp(vAge * historyDecay, 0.0, 0.8));
        }
        float alpha = clamp(1.0 - (vAge * historyDecay), 0.15, 1.0);
        FragColor = vec4(color, alpha);
    }
}
