#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aIntensity;
layout (location = 2) in float aRange;
layout (location = 3) in float aAge;

out float vIntensity;
out float vRange;
out float vAge;

uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    gl_PointSize = mix(6.0, 1.5, aAge); // Point size based on age (newer = bigger)
    vIntensity = aIntensity;
    vRange = aRange;
    vAge = aAge;
}
