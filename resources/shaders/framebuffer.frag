#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;

uniform float exposure = 1.0;
uniform float gamma = 2.2;

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    col += bloomColor;

    vec3 mapped = vec3(1.0) - exp(-col * exposure);
    mapped = pow(mapped, vec3(1.0 / gamma));

    // Output final color
    FragColor = vec4(mapped, 1.0);
} 
