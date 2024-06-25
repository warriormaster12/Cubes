#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormals;


layout (location = 0) out vec3 outNormals;


layout(set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;


void main() {
    outNormals = inNormals;
    gl_Position = camera.proj * camera.view * vec4(inPos, 1.0);
}