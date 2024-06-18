#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

layout(set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;


void main() {
    gl_Position = camera.proj * camera.view * vec4(positions[gl_VertexIndex], 0.0, 1.0);
}