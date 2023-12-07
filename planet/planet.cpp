#include "planet.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

// Lib includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../SOIL2/SOIL2.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <ostream>

// local includes
#include "camera.h"
#include "model.h"
#include "shader.h"

// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// miniaudio
#include <audio.h>

const GLint WIDTH = 1280, HEIGHT = 720;
const double PI = 3.141592653589793238463;
int SCREEN_WIDTH, SCREEN_HEIGHT;

GLFWwindow *window;

// Function prototypes
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
unsigned int loadCubemap(std::vector<std::string> faces);
void doMovement();

// Camera
Camera camera(glm::vec3(40.0f, 5.0f, 150.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
float zNear = 0.1f, zFar = 3500.0f;
bool firstMouse = true;
string cameraType = "";
float volume = 0.5f;

// Light attributes
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat test = 1.0f;
GLfloat scale = 1.0f;
GLfloat AU = 149.597870f; // dividido por 10^6
GLfloat speed = 0.0000001;
GLfloat outerSpeed = 0.000001;

bool menuActive;
bool bloomActive = true;
bool lensFlareActive = true;
bool showPlanetLabels = false;
bool shouldSkip = false;

const char *songs[] = {"resources/others/aphextwin-#20.mp3",
                       "resources/others/bladerunner-rain.mp3",
                       "resources/others/bladerunner-sapperstree.mp3"};

std::vector<glm::vec3> orbitCircle(float radius, int segments) {
  std::vector<glm::vec3> circlePoints;
  for (int i = 0; i < segments; i++) {
    float theta = 2.0f * PI * float(i) / float(segments);
    float x = radius * sin(theta);
    float z = radius * cos(theta);
    circlePoints.push_back(glm::vec3(x, 0.0f, z));
  }
  return circlePoints;
}

struct Sphere {
  glm::vec3 center;
  float radius;
};

Sphere createSphere(float radius, glm::vec3 position) {
  Sphere sphere;
  sphere.center = position;
  sphere.radius = radius;
  return sphere;
}

// Radiuses are in astronomical units
// Rotation speeds are in Km/s
void draw_planet(bool move, int i, glm::mat4 view, glm::mat4 projection,
                 float outerRadius, float innerRadius, float outerRotationSpeed,
                 float innerRotationSpeed, float innerYaw, string name,
                 Shader shader, Shader pathShader, Model planet,
                 Sphere *sphere = NULL, unsigned int nightTextureID = 0,
                 unsigned int cloudTextureID = 0) {
  GLfloat angle, radius, x, y;
  glm::mat4 model(1);
  glm::vec3 pos(0);

  // Rotation around the sun
  if (move) {
    angle = outerRotationSpeed * i;
    radius = outerRadius * AU * scale;
    x = radius * sin(PI * 2 * angle / 360);
    y = radius * cos(PI * 2 * angle / 360);
    model = glm::translate(model, glm::vec3(x, 0.0f, y));
    pos = glm::vec3(x, 0.0f, y);

    if (sphere != NULL)
      sphere->center = glm::vec3(x, 0.0f, y);

    glm::vec3 pathColor = glm::vec3(0.0f, 0.7f, 0.7f); // Red color
    pathShader.use();
    pathShader.setVec3("pathColor", pathColor);

    int segments = 100;
    std::vector<glm::vec3> circlePoints = orbitCircle(radius, segments);
    GLuint vbo, vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, circlePoints.size() * sizeof(glm::vec3),
                 &circlePoints[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                          (void *)0);

    // Draw the orbital path
    shader.use();
    shader.setMat4("model", glm::mat4(1.0f));
    glDrawArrays(GL_LINE_LOOP, 0, circlePoints.size());

    glBindVertexArray(0);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

  } else {
    model = glm::translate(model, glm::vec3(outerRadius * scale, 0.0f, 0.0f));
  }

  if (cameraType == name) {
    if (name == "Mercury" || name == "Venus" || name == "Earth" ||
        name == "Mars" || name == "Neptune") {
      camera.Position =
          (glm::vec3(x + outerRadius + 2.5f, 0.0f, y + outerRadius / 2 + 2.5f));
    }
    if (name == "Jupiter" || name == "Saturn") {
      camera.Position = (glm::vec3(x + outerRadius + 35.0f, 0.0f,
                                   y + outerRadius / 2 + 35.0f));
    }
    if (name == "Uranus") {
      camera.Position = (glm::vec3(x + outerRadius + 10.0f, 0.0f,
                                   y + outerRadius / 2 + 10.0f));
    }
  }

  // Inner rotation
  angle = innerRotationSpeed * i * 1.35;
  model = glm::rotate(model, innerYaw + angle, glm::vec3(0.0f, 0.1f, 0.0f));
  model = glm::scale(model, glm::vec3(innerRadius * scale));
  shader.setMat4("model", model);
  if (name == "Earth") {
    planet.Draw2(shader, "night", nightTextureID, "cloud", cloudTextureID,
                 glfwGetTime());
    return;
  }

  planet.Draw(shader);
  return;
}

bool isIntersecting(glm::vec3 rayOrigin, glm::vec3 rayDirection,
                    const Sphere &sphere, float correction = 1.0f) {

  float cameraToPlanetLength = glm::length(sphere.center - camera.Position);
  float cutoff = cameraToPlanetLength / AU * correction;
  glm::vec3 oc = rayOrigin - sphere.center;
  float a = glm::dot(rayDirection, rayDirection);
  float b = 2.0f * glm::dot(oc, rayDirection);
  float c = glm::dot(oc, oc) -
            (glm::clamp(sphere.radius - cutoff, 0.0f, sphere.radius)) *
                (glm::clamp(sphere.radius - cutoff, 0.0f, sphere.radius));
  float discriminant = b * b - 4 * a * c;

  if (discriminant > 0) {
    // Calculate both possible intersection points
    float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0f * a);

    // Check if the intersection point is between the camera and the sun
    if ((t1 >= 0.0f) || (t2 >= 0.0f)) {
      return true;
    }
  }

  return false;
}

int system() {
  bool move = true;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  window = glfwCreateWindow(WIDTH, HEIGHT, "Solar System", nullptr, nullptr);
  if (nullptr == window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();

    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(window);
  glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);
  glfwSetKeyCallback(window, KeyCallback);
  glfwSetCursorPosCallback(window, MouseCallback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL(
      window, true); // Second param install_callback=true will install GLFW
                     // callbacks and chain to existing ones.

  ImGui::StyleColorsDark();

  ImGui_ImplOpenGL3_Init("#version 330 core");

  glewExperimental = GL_TRUE;
  if (GLEW_OK != glewInit()) {
    std::cout << "Failed to initialize GLEW" << std::endl;
    return EXIT_FAILURE;
  }

  glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glfwSwapInterval(0);

  Shader shader("resources/shaders/modelLoading.vs",
                "resources/shaders/modelLoading.frag");
  Shader earthShader("resources/shaders/earth.vs",
                     "resources/shaders/earth.frag");
  Shader pathShader("resources/shaders/path.vs", "resources/shaders/path.frag");
  Shader skyboxShader("resources/shaders/skybox.vs",
                      "resources/shaders/skybox.frag");
  Shader lampShader("resources/shaders/lamp.vs", "resources/shaders/lamp.frag");
  Shader screenShader("resources/shaders/framebuffer.vs",
                      "resources/shaders/framebuffer.frag");
  Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.frag");

  float lensAperture = 1.0f;
  float sunRadius = 50.0f;
  float earthRadius = 1.5f;
  float mercuryRadius = 0.35f;
  float venusRadius = 1.0f;
  float marsRadius = 0.9f;
  float jupiterRadius = 15.0f - lensAperture;
  float saturnRadius = 12.0f - lensAperture;
  float uranusRadius = 10.0f - lensAperture;
  float neptuneRadius = 10.0f - lensAperture;
  // Load models
  Model earthModel("resources/models/earth/earth.obj");
  Model sunModel("resources/models/sun/sun.obj");
  Model mercuryModel("resources/models/mercury/mercury.obj");
  Model venusModel("resources/models/venus/venus.obj");
  Model marsModel("resources/models/mars/mars.obj");
  Model jupiterModel("resources/models/jupiter/jupiter.obj");
  Model saturnModel("resources/models/saturn/saturn.obj");
  Model uranusModel("resources/models/uranus/uranus.obj");
  Model neptuneModel("resources/models/neptune/neptune.obj");

  // Spheres for ray cast collisions
  Sphere sunSphere = createSphere(sunRadius, lightPos);
  Sphere earthSphere =
      createSphere(earthRadius, glm::vec3(0.0f, 0.0f, 1.0f * AU));
  Sphere mercurySphere =
      createSphere(mercuryRadius, glm::vec3(0.0f, 0.0f, 0.39f * AU));
  Sphere venusSphere =
      createSphere(venusRadius, glm::vec3(0.0f, 0.0f, 0.72f * AU));
  Sphere marsSphere =
      createSphere(marsRadius, glm::vec3(0.0f, 0.0f, 1.52f * AU));
  Sphere jupiterSphere =
      createSphere(jupiterRadius, glm::vec3(0.0f, 0.0f, 5.2f * AU));
  Sphere saturnSphere =
      createSphere(saturnRadius, glm::vec3(0.0f, 0.0f, 9.54f * AU));
  Sphere uranusSphere =
      createSphere(jupiterRadius, glm::vec3(0.0f, 0.0f, 14.22f * AU));
  Sphere neptuneSphere =
      createSphere(jupiterRadius, glm::vec3(0.0f, 0.0f, 23.06f * AU));

  unsigned int earthNightTextureID =
      TextureFromFile("resources/models/earth/earthnight.jpg", ".");

  unsigned int earthCloudTextureID =
      TextureFromFile("resources/models/earth/earthclouds.jpg", ".");

  unsigned int noiseTextureID =
      TextureFromFile("resources/models/others/noise.png", ".");
  //    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

  std::thread audioThread(&initializeMiniaudio);

  // Set light properties
  shader.use();
  shader.setVec3("light.position", lightPos);
  shader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
  shader.setVec3("light.diffuse", 1.5f, 1.5f, 1.5f);
  shader.setVec3("light.specular", 0.3f, 0.3f, 0.3f);
  shader.setFloat("light.constant", 1.5f);
  shader.setFloat("light.linear", 0.0000002f);
  shader.setFloat("light.quadratic", 0.0000006f);

  earthShader.use();
  earthShader.setVec3("light.position", lightPos);
  earthShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
  earthShader.setVec3("light.diffuse", 1.5f, 1.5f, 1.5f);
  earthShader.setVec3("light.specular", 0.3f, 0.3f, 0.3f);
  earthShader.setFloat("light.constant", 1.0f);
  earthShader.setFloat("light.linear", 0.0000002f);
  earthShader.setFloat("light.quadratic", 0.0000006f);

  lampShader.use();
  lampShader.setFloat("sunIntensity", 200.5f);

  float skyboxVertices[] = {
      // positions
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

      1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

      -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

      -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

  /* SKYBOX GENERATION */
  unsigned int skyboxVAO, skyboxVBO;
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);
  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  /* SKYBOX GENERATION */

  std::vector<std::string> faces{
      "resources/skybox/bkg1_right.png", "resources/skybox/bkg1_left.png",
      "resources/skybox/bkg1_top.png",   "resources/skybox/bkg1_bot.png",
      "resources/skybox/bkg1_front.png", "resources/skybox/bkg1_back.png",
  };

  // Post processing effects
  float quadVertices[] = {// vertex attributes for a quad that fills the entire
                          // screen in Normalized Device Coordinates.
                          // positions   // texCoords
                          -1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                          0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                          -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                          1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};
  // screen quad VAO
  unsigned int quadVAO, quadVBO;
  glGenVertexArrays(1, &quadVAO);
  glGenBuffers(1, &quadVBO);
  glBindVertexArray(quadVAO);
  glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                        (void *)(2 * sizeof(float)));

  screenShader.use();
  screenShader.setInt("screenTexture", 0);
  screenShader.setInt("bloomBlur", 1);
  screenShader.setVec3("lightPos", lightPos);
  screenShader.setInt("screen_width", SCREEN_WIDTH);
  screenShader.setInt("screen_height", SCREEN_HEIGHT);

  glActiveTexture(GL_TEXTURE0);
  screenShader.setInt("noise_texture", 0);
  glBindTexture(GL_TEXTURE_2D, noiseTextureID);

  unsigned int framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  // create a color attachment texture
  unsigned int textureColorbuffer;
  glGenTextures(1, &textureColorbuffer);
  glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
               GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         textureColorbuffer, 0);

  unsigned int bloomTexture;
  glGenTextures(1, &bloomTexture);
  glBindTexture(GL_TEXTURE_2D, bloomTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
               GL_RGB, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                         bloomTexture, 0);

  // create a renderbuffer object for depth and stencil attachment (we won't be
  // sampling these)
  unsigned int rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCREEN_WIDTH,
                        SCREEN_HEIGHT); // use a single renderbuffer object for
                                        // both a depth AND stencil buffer.
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                            GL_RENDERBUFFER, rbo); // now actually attach it

  unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
  glDrawBuffers(2, attachments);

  // now that we actually created the framebuffer and added all attachments we
  // want to check if it is actually complete now
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // ping-pong-framebuffer for gaussian blurring
  unsigned int pingpongFBO[2];
  unsigned int pingpongColorbuffers[2];
  glGenFramebuffers(2, pingpongFBO);
  glGenTextures(2, pingpongColorbuffers);
  for (unsigned int i = 0; i < 2; i++) {
    glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0,
                 GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           pingpongColorbuffers[i], 0);
    // also check if framebuffers are complete (no need for depth buffer)
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      std::cout << "Framebuffer not complete!" << std::endl;
  }

  blurShader.use();
  blurShader.setInt("image", 0);

  unsigned int cubemapTexture = loadCubemap(faces);
  GLuint i = 0;
  while (!glfwWindowShouldClose(window)) {

    GLfloat currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (menuActive) {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    } else {
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {

      ImGui::Begin("Menu"); // Cria o menu incial
      // Edit bools storing our window open/close state
      ImGui::SliderFloat("Rotation Speed", &outerSpeed, 0.0f,
                         0.1f); // Altera a velocidade de movimento do planetas

      ImGui::SliderFloat("Camera Speed", &camera.MovementSpeed, 0.0, 500.0);

      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Planets");

      if (ImGui::Button("Mercury")) {
        cameraType = "Mercury";
      }
      ImGui::SameLine();
      if (ImGui::Button("Venus")) {
        cameraType = "Venus";
      }
      ImGui::SameLine();
      if (ImGui::Button("Earth")) {
        cameraType = "Earth";
      }
      ImGui::SameLine();
      if (ImGui::Button("Mars")) {
        cameraType = "Mars";
      }
      if (ImGui::Button("Jupiter")) {
        cameraType = "Jupiter";
      }
      ImGui::SameLine();
      if (ImGui::Button("Saturn")) {
        cameraType = "Saturn";
      }
      ImGui::SameLine();
      if (ImGui::Button("Uranus")) {
        cameraType = "Uranus";
      }
      ImGui::SameLine();
      if (ImGui::Button("Neptune")) {
        cameraType = "Neptune";
      }

      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Simulation Settings");
      if (ImGui::Button("Bloom")) {
        bloomActive = !bloomActive;
      }
      ImGui::SameLine();
      if (ImGui::Button("Lens Flare")) {
        lensFlareActive = !lensFlareActive;
      }

      ImGui::TextColored(ImVec4(1, 1, 0, 1), "Audio Controls");
      ImGui::SliderFloat("Audio Volume", &volume, 0.0, 1.0f);

      if (ImGui::Button("Skip Song")) {
        shouldSkip = true;
      }

      ImGui::End();
    }

    i++;
    doMovement();

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.00f, 0.00f, 0.00f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, zNear, zFar);

    glm::mat4 model(1);

    shader.use();
    shader.setVec3("viewPos", camera.Position);
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // MERCURY
    draw_planet(move, i, view, projection, 0.39f, 1.0f, 49.9f * outerSpeed,
                10.83f * speed, 0.0f, "Mercury", shader, pathShader,
                mercuryModel, &mercurySphere);

    // VENUS
    draw_planet(move, i, view, projection, 0.72f, 1.0f, 35.0f * outerSpeed,
                6.52f * speed, 0.0f, "Venus", shader, pathShader, venusModel,
                &venusSphere);
    // EARTH

    earthShader.use();
    earthShader.setVec3("viewPos", camera.Position);
    earthShader.setMat4("projection", projection);
    earthShader.setMat4("view", view);
    draw_planet(move, i, view, projection, 1.0f, 1.4f, 29.8f * outerSpeed,
                1574.0f * speed, 0.0f, "Earth", earthShader, pathShader,
                earthModel, &earthSphere, earthNightTextureID,
                earthCloudTextureID);

    // MARS
    draw_planet(move, i, view, projection, 1.52f, 1.0f, 24.1f * outerSpeed,
                866.0f * speed, 0.0f, "Mars", shader, pathShader, marsModel,
                &marsSphere);

    // JUPITER
    draw_planet(move, i, view, projection, 5.20f, 1.0f, 13.1f * outerSpeed,
                45583.0f * speed, 0.0f, "Jupiter", shader, pathShader,
                jupiterModel, &jupiterSphere);

    // SATURN
    draw_planet(move, i, view, projection, 9.54f, 1.0f, 9.7f * outerSpeed,
                36840.0f * speed, 90.0f, "Saturn", shader, pathShader,
                saturnModel, &saturnSphere);

    // Uranus
    draw_planet(move, i, view, projection, 14.22f, 1.0f, 6.8f * outerSpeed,
                14797.0f * speed, 160.0f, "Uranus", shader, pathShader,
                uranusModel, &uranusSphere);

    // NEPTUNE
    draw_planet(move, i, view, projection, 23.06f, 1.0f, 5.4f * outerSpeed,
                9719.0f * speed, 130.0f, "Neptune", shader, pathShader,
                neptuneModel, &neptuneSphere);

    // SUN
    lampShader.use();
    lampShader.setMat4("view", view);
    lampShader.setMat4("projection", projection);

    model = glm::mat4(1);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(scale));

    glm::mat4 sunVp = projection * view;
    glm::vec4 sunClipCoords = sunVp * glm::vec4(lightPos, 1.0);
    sunClipCoords /= sunClipCoords.w;

    glm::vec3 sunScreenPos =
        glm::vec3((sunClipCoords.x + 1.0f) * 0.5f * SCREEN_WIDTH,
                  (sunClipCoords.y + 1.0f) * 0.5f * SCREEN_HEIGHT,
                  (sunClipCoords.z + 1.0f) * 0.5f);

    shader.setMat4("model", model);
    lampShader.setMat4("model", model);

    sunModel.Draw(lampShader);

    if (cameraType == "Up") {
      camera.Position = (glm::vec3(-1.438195, 38.160343, 1.159209));
    }

    /* DRAW SKYBOX */
    glDepthFunc(GL_LEQUAL);
    skyboxShader.use();
    view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
    skyboxShader.setMat4("view", view);
    skyboxShader.setMat4("projection", projection);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    /* DRAW SKYBOX */

    bool horizontal = true, first_iteration = true;
    unsigned int amount = 10;
    blurShader.use();
    for (unsigned int i = 0; i < amount; i++) {
      glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
      blurShader.setInt("horizontal", horizontal);
      glBindTexture(
          GL_TEXTURE_2D,
          first_iteration
              ? bloomTexture
              : pingpongColorbuffers[!horizontal]); // bind texture of other
                                                    // framebuffer (or scene if
                                                    // first iteration)
      glBindVertexArray(quadVAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);

      horizontal = !horizontal;
      first_iteration = false;
    }

    // now bind back to default framebuffer and draw a quad plane with the
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /* bool sunVisible = isSunVisible(bloomTexture, 0.0f, 0.0f, 0.0f, 1.0f); */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    screenShader.use();
    // Assuming clipSpacePosition is a glm::vec4
    screenShader.setMat4("view", view);
    screenShader.setMat4("projection", projection);
    screenShader.setVec3("screenLightPos", glm::vec3(sunScreenPos));
    screenShader.setBool("bloomActive", bloomActive);

    // ray casting para saber se o sol esta obstruido
    glm::vec3 rayDirection = glm::normalize(sunSphere.center - camera.Position);
    float rayLength = glm::length(sunSphere.center - camera.Position);
    glm::vec3 rayEndPoint = camera.Position + rayDirection * rayLength;
    if (lensFlareActive) {
      bool sunVisible =
          !isIntersecting(camera.Position, rayDirection, mercurySphere,
                          10.0f) &&
          !isIntersecting(camera.Position, rayDirection, venusSphere, 2.0f) &&
          !isIntersecting(camera.Position, rayDirection, earthSphere, 2.0f) &&
          !isIntersecting(camera.Position, rayDirection, marsSphere) &&
          !isIntersecting(camera.Position, rayDirection, jupiterSphere, 1.5f) &&
          !isIntersecting(camera.Position, rayDirection, saturnSphere) &&
          !isIntersecting(camera.Position, rayDirection, uranusSphere) &&
          !isIntersecting(camera.Position, rayDirection, neptuneSphere);
      screenShader.setBool("sunVisibleAndEnabled", sunVisible);
    } else {
      screenShader.setBool("sunVisibleAndEnabled", false);
    }

    glBindVertexArray(quadVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  audioThread.join();
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}

void doMovement() {

  if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) {
    camera.ProcessKeyboard(FORWARD, deltaTime);
  }

  if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) {
    camera.ProcessKeyboard(BACKWARD, deltaTime);
  }

  if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT]) {
    camera.ProcessKeyboard(LEFT, deltaTime);
  }

  if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) {
    camera.ProcessKeyboard(RIGHT, deltaTime);
  }

  if (keys[GLFW_KEY_P]) {
    menuActive = true;
  }

  if (keys[GLFW_KEY_O]) {
    menuActive = false;
  }

  /* if (keys[GLFW_KEY_MINUS]) { */
  /*   camera.DecreaseSpeed(); */
  /* } */
  /**/
  /* if (keys[GLFW_KEY_EQUAL]) { */
  /*   camera.IncreaseSpeed(); */
  /* } */

  if (keys[GLFW_KEY_1]) {
    cameraType = "Mercury";
    return;
  } else if (keys[GLFW_KEY_2]) {
    cameraType = "Venus";
    return;
  } else if (keys[GLFW_KEY_3]) {
    cameraType = "Earth";
    return;
  } else if (keys[GLFW_KEY_4]) {
    cameraType = "Mars";
    return;
  } else if (keys[GLFW_KEY_5]) {
    cameraType = "Jupiter";
    return;
  } else if (keys[GLFW_KEY_6]) {
    cameraType = "Saturn";
    return;
  } else if (keys[GLFW_KEY_7]) {
    cameraType = "Uranus";
    return;
  } else if (keys[GLFW_KEY_8]) {
    cameraType = "Neptune";
    return;
  } else if (keys[GLFW_KEY_0]) {
    cameraType = "";
    return;
  } else if (keys[GLFW_KEY_U]) {
    cameraType = "Up";
    return;
  }

  if (keys[GLFW_KEY_M]) {
    speed += 0.000001;
    outerSpeed += 0.00001;
    std::cout << "SPEED (simulation hours/s) : " << (speed) << std::endl;
    return;
  } else if (keys[GLFW_KEY_N]) {
    speed -= 0.000001;
    outerSpeed -= 0.00001;
    std::cout << "SPEED (simulation hours/s): " << (speed) << std::endl;
    return;
  }

  else if (keys[GLFW_KEY_B]) {
    speed = 0.004380;
    outerSpeed = 0.04380;
    std::cout << "SPEED (simulation hours/s) : " << (speed) << std::endl;
    return;
  }
  return;
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mode) {
  if (GLFW_KEY_ESCAPE == key && GLFW_PRESS == action) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }

  if (key >= 0 && key < 1024) {
    if (action == GLFW_PRESS) {
      keys[key] = true;
    } else if (action == GLFW_RELEASE) {
      keys[key] = false;
    }
  }
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos) {
  if (firstMouse) {
    lastX = xPos;
    lastY = yPos;
    firstMouse = false;
  }

  GLfloat xOffset = xPos - lastX;
  GLfloat yOffset = lastY - yPos;

  lastX = xPos;
  lastY = yPos;

  if (!menuActive)
    camera.ProcessMouseMovement(xOffset, yOffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  camera.ProcessMouseScroll(static_cast<float>(-yoffset));
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

unsigned int loadCubemap(std::vector<std::string> faces) {
  unsigned int textureID;
  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

  int width, height, nrChannels;

  for (unsigned int i = 0; i < faces.size(); i++) {

    unsigned char *data =
        stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_SRGB, width,
                   height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      stbi_image_free(data);
    } else {
      std::cout << "Cubemap texture failed to load at path: " << faces[i]
                << std::endl;
      stbi_image_free(data);
    }
  }
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  return textureID;
}
