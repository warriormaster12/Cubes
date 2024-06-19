#version 450

layout (location = 0) in vec2 inPos;


layout(set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;


void main() {
    gl_Position = camera.proj * camera.view * vec4(inPos, 0.0, 1.0);
}