#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormals;


layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormals;


layout(set = 0, binding = 0) uniform Camera {
    mat4 proj;
    mat4 view;
} camera;

layout(std140, set = 1, binding = 0) readonly buffer ObjectData {
    mat4 transforms[];
} objectData;

void main() {
    mat4 currentTransform = objectData.transforms[gl_InstanceIndex];
    outNormals = mat3(transpose(inverse(currentTransform))) * inNormals;
    outFragPos = vec3(currentTransform * vec4(inPos, 1.0));
    gl_Position = camera.proj * camera.view * vec4(outFragPos, 1.0);
}