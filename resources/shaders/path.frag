#version 330 core
out vec4 FragColor;

uniform vec3 pathColor;

void main() {
    FragColor = vec4(pathColor, 1.0);
}
