#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Material {
    vec4 color;
} material;

void main() {
    outColor = material.color;
}