#version 330 core

in float vIntensity;
in float vRange;
in float vAge;

out vec4 FragColor;

uniform float maxRange;
uniform int colorMode; // 0 for Range, 1 for Intensity
uniform float historyDecay;

// Turbo colormap approximation
vec3 colormapTurbo(float x) {
    float r = 0.137932 + x * ( 3.586736 + x * (-7.367018 + x * ( 6.162791 + x * -1.536647)));
    float g = 0.096738 + x * ( 2.158739 + x * (-1.464670 + x * ( 0.103099 + x * -0.063231)));
    float b = 0.231920 + x * (-1.011681 + x * ( 5.433290 + x * (-7.644498 + x *  3.181827)));
    return vec3(r, g, b);
}

void main() {
    float t = 0.0;
    if (colorMode == 0) {
        t = clamp(vRange / maxRange, 0.0, 1.0);
    } else {
        t = clamp(vIntensity / 255.0, 0.0, 1.0); // Assuming 8-bit intensity
    }
    
    vec3 color = colormapTurbo(t);
    // Historical points are bluer and fade out based on historyDecay
    if (vAge > 0.01) {
        color = mix(color, vec3(0.2, 0.4, 0.9), clamp(vAge * historyDecay, 0.0, 0.8));
    }
    float alpha = clamp(1.0 - (vAge * historyDecay), 0.1, 1.0);
    FragColor = vec4(color, alpha);
}
