#version 450

layout(location = 0) in vec3 inNormals;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform Material {
    vec4 color;
} material;

void main() {
    outColor = vec4(inNormals, 1.0);
}