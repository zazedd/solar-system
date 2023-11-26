
#version 330 core

in vec2 TexCoords;

struct Light {
    vec3 position;
    
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    
    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;
in vec3 Normal;

out vec4 color;

uniform vec3 viewPos;
uniform sampler2D texture_diffuse;
uniform sampler2D night;
uniform sampler2D texture_specular;
uniform Light light;

void main() {
  // 0 -> day, 1 -> night
  // Ambient
  vec3 ambient = light.ambient * vec3(texture(texture_diffuse, TexCoords));

  // Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse, TexCoords));

  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
  vec3 specular = light.specular * spec * vec3(texture( texture_specular, TexCoords));

  float distance    = length(light.position - FragPos);
  float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation;

  vec3 t0 = ambient + diffuse + specular;
  vec3 t1 = texture(night, TexCoords).rgb;

  float NdotL = dot(norm, lightDir);

  float y = smoothstep(-0.15, 0.15, NdotL);
  color = vec4(t0 * y + t1 * (1 - y), 1.0);
}
