
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
uniform sampler2D cloud; 
uniform Light light;

uniform float time;

void main() {
  // 0 -> day, 1 -> night

  vec2 cloudCoords = TexCoords;

  float offset = 0.005 * time;
  cloudCoords.x = fract(cloudCoords.x + offset);

  // Ambient
  vec3 ambient = light.ambient * vec3(texture(texture_diffuse, TexCoords));
  vec3 cloudAmbient = light.ambient * vec3(texture(cloud, cloudCoords));

  // Diffuse
  vec3 norm = normalize(Normal);
  vec3 lightDir = normalize(light.position - FragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse, TexCoords));
  vec3 cloudDiffuse = light.diffuse * diff * vec3(texture(cloud, cloudCoords));

  // Specular
  vec3 viewDir = normalize(viewPos - FragPos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0f);
  vec3 specular = light.specular * spec * vec3(texture(texture_specular, TexCoords));
  vec3 cloudSpecular = light.specular * spec * vec3(texture(cloud, cloudCoords));

  float distance    = length(light.position - FragPos);
  float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

  ambient  *= attenuation;
  diffuse  *= attenuation;
  specular *= attenuation;

  cloudAmbient  *= attenuation;
  cloudDiffuse  *= attenuation;
  cloudSpecular *= attenuation;

  vec3 t0 = ambient + diffuse + specular;
  vec3 t1 = texture(night, TexCoords).rgb;
  vec3 cloudColor = cloudAmbient + cloudDiffuse + cloudSpecular;

  float NdotL = dot(norm, lightDir);

  float y = smoothstep(-0.15, 0.15, NdotL);
  color = vec4(t0 * y + (t1 * (1 - y)) + (cloudColor * 0.5), 1.0); 
}
