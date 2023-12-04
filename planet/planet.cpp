#include "planet.hpp"
#include <iostream>

// Lib includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../SOIL2/SOIL2.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ostream>

// local includes
#include "camera.h"
#include "model.h"
#include "shader.h"

// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const GLint WIDTH = 1400, HEIGHT = 800;
const double PI = 3.141592653589793238463;
int SCREEN_WIDTH, SCREEN_HEIGHT;

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

// Light attributes
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat test = 1.0f;
GLfloat scale = 1.0f;
GLfloat AU = 149.597870f; // dividido por 10^6
GLfloat speed = 0.0000001;
GLfloat outerSpeed = 0.000001;

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

// Radiuses are in astronomical units
// Rotation speeds are in Km/s
void draw_planet(bool move, int i, float outerRadius, float innerRadius,
                 float outerRotationSpeed, float innerRotationSpeed,
                 float innerYaw, string name, Shader shader, Shader pathShader,
                 Model planet, unsigned int nightTextureID = 0,
                 unsigned int cloudTextureID = 0) {
  GLfloat angle, radius, x, y;
  glm::mat4 model(1);

  // Rotation around the sun
  if (move) {
    angle = outerRotationSpeed * i;
    radius = outerRadius * AU * scale;
    x = radius * sin(PI * 2 * angle / 360);
    y = radius * cos(PI * 2 * angle / 360);
    model = glm::translate(model, glm::vec3(x, 0.0f, y));

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
    camera.Position =
        (glm::vec3(x + outerRadius + 1.5f, 0.0f, y + outerRadius / 2 + 1.5f));
  }

  // Inner rotation
  angle = innerRotationSpeed * i * 1.35;
  model = glm::rotate(model, innerYaw + angle, glm::vec3(0.0f, 0.1f, 0.0f));
  model = glm::scale(model, glm::vec3(innerRadius * scale));
  shader.setMat4("model", model);
  if (name == "Earth") {
    planet.Draw2(shader, "night", nightTextureID, "cloud", cloudTextureID,
                 glfwGetTime());
    printf("%f\n", glfwGetTime());
    return;
  }

  planet.Draw(shader);
  return;
}

int system() {
  bool move = true;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(WIDTH, HEIGHT, "Solar System", nullptr, nullptr);

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

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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
  Shader directionalShader("resources/shaders/directional.vs",
                           "resources/shaders/directional.frag");
  Shader pathShader("resources/shaders/path.vs", "resources/shaders/path.frag");
  Shader skyboxShader("resources/shaders/skybox.vs",
                      "resources/shaders/skybox.frag");
  Shader lampShader("resources/shaders/lamp.vs", "resources/shaders/lamp.frag");

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

  unsigned int earthNightTextureID =
      TextureFromFile("resources/models/earth/earthnight.jpg", ".");

  unsigned int earthCloudTextureID =
      TextureFromFile("resources/models/earth/earthclouds.jpg", ".");
  //    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

  // Set light properties
  directionalShader.use();
  directionalShader.setVec3("light.position", lightPos);
  directionalShader.setVec3("light.ambient", 0.2f, 0.2f, 0.2f);
  directionalShader.setVec3("light.diffuse", 1.5f, 1.5f, 1.5f);
  directionalShader.setVec3("light.specular", 0.3f, 0.3f, 0.3f);

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

  unsigned int cubemapTexture = loadCubemap(faces);
  GLuint i = 0;
  while (!glfwWindowShouldClose(window)) {
    GLfloat currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    i++;
    doMovement();
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();

    glm::mat4 projection = glm::perspective(
        glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, zNear, zFar);

    directionalShader.use();
    // Give projection and view matrices to light shader
    directionalShader.setMat4("projection", projection);
    directionalShader.setMat4("view", view);

    // Model

    // SPACE
    glm::mat4 model(1);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(20.0f));
    directionalShader.setMat4("model", model);

    shader.use();
    shader.setVec3("viewPos", camera.Position);
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // MERCURY
    draw_planet(move, i, 0.39f, 1.0f, 49.9f * outerSpeed, 10.83f * speed, 0.0f,
                "Mercury", shader, pathShader, mercuryModel);

    // VENUS
    draw_planet(move, i, 0.72f, 1.0f, 35.0f * outerSpeed, 6.52f * speed, 0.0f,
                "Venus", shader, pathShader, venusModel);
    // EARTH

    earthShader.use();
    earthShader.setVec3("viewPos", camera.Position);
    earthShader.setMat4("projection", projection);
    earthShader.setMat4("view", view);
    draw_planet(move, i, 1.0f, 1.4f, 29.8f * outerSpeed, 1574.0f * speed, 0.0f,
                "Earth", earthShader, pathShader, earthModel,
                earthNightTextureID, earthCloudTextureID);

    // MARS
    draw_planet(move, i, 1.52f, 1.0f, 24.1f * outerSpeed, 866.0f * speed, 0.0f,
                "Mars", shader, pathShader, marsModel);

    // JUPITER
    draw_planet(move, i, 5.20f, 1.0f, 13.1f * outerSpeed, 45583.0f * speed,
                0.0f, "Jupiter", shader, pathShader, jupiterModel);

    // SATURN
    draw_planet(move, i, 9.54f, 1.0f, 9.7f * outerSpeed, 36840.0f * speed,
                90.0f, "Saturn", shader, pathShader, saturnModel);

    // Uranus
    draw_planet(move, i, 14.22f, 1.0f, 6.8f * outerSpeed, 14797.0f * speed,
                160.0f, "Uranus", shader, pathShader, uranusModel);

    // NEPTUNE
    draw_planet(move, i, 23.06f, 1.0f, 5.4f * outerSpeed, 9719.0f * speed,
                130.0f, "Neptune", shader, pathShader, neptuneModel);

    // SUN
    lampShader.use();
    lampShader.setMat4("view", view);
    lampShader.setMat4("projection", projection);

    model = glm::mat4(1);
    model = glm::translate(model, lightPos);
    model = glm::scale(model, glm::vec3(scale));
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

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

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
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                   0, GL_RGB, GL_UNSIGNED_BYTE, data);
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
