#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in float aIntensity;
layout (location = 2) in float aRange;
layout (location = 3) in float aAge;

out float vIntensity;
out float vRange;
out float vAge;

uniform mat4 projection;
uniform float pointSize;   // base point size from UI slider
uniform bool  circularPts; // whether to use circular discard

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    // Newer points (age=0) are full size; old (age=1) shrink to 30%
    gl_PointSize = mix(pointSize, pointSize * 0.3, aAge);
    vIntensity = aIntensity;
    vRange = aRange;
    vAge = aAge;
}
