#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in vec2 TexCoords;

out vec4 color;

uniform sampler2D texture_diffuse;
uniform float sunIntensity;

void main() {
  color = texture(texture_diffuse, TexCoords) * sunIntensity;
  FragColor = color;

  float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
  if(brightness > 1.0)
    BrightColor = vec4(color.rgb, 1.0);
  else
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
