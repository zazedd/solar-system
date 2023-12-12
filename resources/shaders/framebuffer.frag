#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec2 fragViewDir;

uniform sampler2D screenTexture;
uniform sampler2D bloomBlur;
uniform sampler2D noise_texture;
uniform vec3 tint = vec3(1.4,1.2,1.0);
// uniform sampler2D noise_texture;
uniform int screen_width;
uniform int screen_height;
uniform vec3 screenLightPos; 
uniform bool sunVisibleAndEnabled; 
uniform bool bloomActive; 

uniform float exposure = 0.4;
uniform float gamma = 2.2;

float noise(float t)
{
  vec2 noiseResolution = textureSize(noise_texture, 0);
	return texture(noise_texture, vec2(t,.0) / noiseResolution).x;
}

float noise(vec2 t)
{
  vec2 noiseResolution = textureSize(noise_texture, 0);
	return texture(noise_texture, t /noiseResolution).x;
}

vec3 lensflare(vec2 uv,vec2 pos)
{
	vec2 main = uv-pos;
	vec2 uvd = uv*(length(uv));
	
  float ang = atan(main.x, main.y);
	float dist=length(main); dist = pow(dist,.1);
	float n = noise(vec2(ang*16.0,dist*32.0));
	
	float f0 = 1.0/(length(uv-pos)*50.0+1.0);
	
	f0 = f0 + f0*(sin(noise(sin(ang*2.+pos.x)*4.0 - cos(ang*3.+pos.y))*16.)*.1 + dist*.1 + .8);
	
  float f1 = max(0.01-pow(length(uv+1.2*pos),1.9),.0)*4.0;

  vec2 posCopy = pos;
  float dist2 = 0.35;

	float f2 = max(1.0/(1.0+32.0*pow(length(uvd+0.8*pos),1.2)),.0)*00.25;
	float f22 = max(1.0/(1.0+32.0*pow(length(uvd+0.85*pos),1.2)),.0)*00.23;
	float f23 = max(1.0/(1.0+32.0*pow(length(uvd+0.9*pos),1.2)),.0)*00.21;
	
	vec2 uvx = mix(uv,uvd,-0.5);

	
	float f4 = max(0.01-pow(length(uvx+0.4*pos),2.4),.0)*4.0;
	float f42 = max(0.01-pow(length(uvx+0.45*pos),2.4),.0)*3.0;
	float f43 = max(0.01-pow(length(uvx+0.5*pos),2.4),.0)*2.0;
	
	uvx = mix(uv,uvd,-.4);

  dist2 = 2.0;
  pos = posCopy;
  pos *= dist2;
	
	float f5 = max(0.01-pow(length(uvx+0.2*pos),5.5),.0)*1.0;
	float f52 = max(0.01-pow(length(uvx+0.4*pos),5.5),.0)*1.0;
	float f53 = max(0.01-pow(length(uvx+0.6*pos),5.5),.0)*1.0;
	
	uvx = mix(uv,uvd,-0.5);
	
	float f6 = max(0.01-pow(length(uvx-0.3*pos),1.6),.0)*4.0;
	float f62 = max(0.01-pow(length(uvx-0.325*pos),1.6),.0)*2.0;
	float f63 = max(0.01-pow(length(uvx-0.35*pos),1.6),.0)*3.0;
	
	vec3 c = vec3(.0);
	
	c.r+=f2+f4+f5+f6; c.g+=f22+f42+f52+f62; c.b+=f23+f43+f53+f63;
	c = c*1.3 - vec3(length(uvd)*.05);
	c+=vec3(f0);
	
	return c;
}

vec3 cc(vec3 color, float factor,float factor2) // color modifier
{
	float w = color.x+color.y+color.z;
	return mix(color,vec3(w)*factor,w*factor2);
}

vec3 LensFlare() {

  vec2 uv = TexCoords - 0.5;
	uv.x *= screen_width / screen_height; //fix aspect ratio
	vec2 mouse = screenLightPos.xy / vec2(screen_width, screen_height) - 0.5;
	mouse.x *= screen_width / screen_height; //fix aspect ratio
	
	vec3 color = vec3(1.4,1.2,1.0)*lensflare(uv, mouse.xy);
	return cc(color,.5,.1);
}

void main()
{
  vec3 col = texture(screenTexture, TexCoords).rgb;
  vec4 bloomTex = texture(bloomBlur, TexCoords);
  vec3 bloomColor = bloomTex.rgb;

  if (bloomActive)
    col += bloomColor;

  float lum = dot(col, vec3(0.2126, 0.7152, 0.0722));

  const float adjSpeed = 0.05;
  float targetExposure = 0.5 / lum * 3.0f; 
  float sceneExposure = mix(exposure, targetExposure, adjSpeed);

  sceneExposure = clamp(exposure, 0.3f, 3.0f);

  vec3 mapped = vec3(1.0) - exp(-col * sceneExposure);
  mapped = pow(mapped, vec3(1.0 / gamma));

  if (screenLightPos.z < 1 && sunVisibleAndEnabled)
    mapped += LensFlare();

  // Output final color
  FragColor = vec4(mapped, 1.0);
} 

