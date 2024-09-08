#version 450

layout (location = 0) in vec3 inFragPos;
layout(location = 1) in vec3 inNormals;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 1) uniform Material {
    vec4 color;
} material;

void main() {
    vec3 lightPos = vec3(0.0, 0.0, 5.0);
    //ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);

    //diffuse
    vec3 norm = normalize(inNormals);
    vec3 lightDir = normalize(lightPos - inFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);

    vec3 result = (ambient + diffuse) * vec3(material.color);
    outColor = vec4(result, material.color.a);
}