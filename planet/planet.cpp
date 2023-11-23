#include "planet.hpp"
#include <iostream>

// Lib includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../SOIL2/SOIL2.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// local includes
#include "camera.h"
#include "model.h"
#include "shader.h"

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
void DoMovement();

// Camera
Camera camera(glm::vec3(40.0f, 0.0f, 30.0f));
bool keys[1024];
GLfloat lastX = 400, lastY = 300;
float zNear = 0.1f, zFar = 1000.0f;
bool firstMouse = true;
string cameraType = "";

// Light attributes
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

GLfloat speed = 1.0f;
GLfloat scale = 0.1f;
GLfloat AU = 149.597870f;

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
void draw_planet(bool move, int i, float outerRadius, float innerRadius,
                 float outerRotationSpeed, float innerRotationSpeed,
                 float innerYaw, string name, Shader shader, Shader pathShader,
                 Model planet) {
  GLfloat angle, radius, x, y;
  glm::mat4 model(1);

  // Rotation around the sun
  if (move) {
    angle = outerRotationSpeed * i * speed;
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
    camera.Position = (glm::vec3(x + 0.5f, 0.0f, y + 0.5f));
  }

  // Inner rotation
  angle = innerRotationSpeed * i;
  model = glm::rotate(model, innerYaw + angle, glm::vec3(0.0f, 0.1f, 0.0f));
  model = glm::scale(model, glm::vec3(innerRadius * scale));
  shader.setMat4("model", model);
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
  shader.setFloat("light.constant", 1.0f);
  shader.setFloat("light.linear", 0.00002f);
  shader.setFloat("light.quadratic", 0.000006f);

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
    DoMovement();
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
    draw_planet(move, i, 0.39f, 1.0f, 0.0008f, 0.0f, 0.0f, "Mercury", shader,
                pathShader, mercuryModel);

    // VENUS
    draw_planet(move, i, 0.72f, 1.0f, 0.007f, 0.0f, 0.0f, "Venus", shader,
                pathShader, venusModel);
    // EARTH
    draw_planet(move, i, 1.0f, 1.0f, 0.006f, 0.001f, 0.0f, "Earth", shader,
                pathShader, earthModel);

    // MARS
    draw_planet(move, i, 1.52f, 1.0f, 0.005f, 0.0f, 0.0f, "Mars", shader,
                pathShader, marsModel);

    // JUPITER
    draw_planet(move, i, 5.20f, 1.0f, 0.005f, 0.0f, 0.0f, "Jupiter", shader,
                pathShader, jupiterModel);

    // SATURN
    draw_planet(move, i, 9.54f, 1.0f, 0.004f, 0.0001f, 90.0f, "Saturn", shader,
                pathShader, saturnModel);

    // Uranus
    draw_planet(move, i, 19.22f, 1.0f, 0.0035f, 0.00001f, 160.0f, "Uranus",
                shader, pathShader, uranusModel);

    // NEPTUNE
    draw_planet(move, i, 30.06f, 1.0f, 0.003f, 0.00001f, 130.0f, "Neptune",
                shader, pathShader, neptuneModel);

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

void DoMovement() {

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
  } else if (keys[GLFW_KEY_2]) {
    cameraType = "Venus";
  } else if (keys[GLFW_KEY_3]) {
    cameraType = "Earth";
  } else if (keys[GLFW_KEY_4]) {
    cameraType = "Mars";
  } else if (keys[GLFW_KEY_5]) {
    cameraType = "Jupiter";
  } else if (keys[GLFW_KEY_6]) {
    cameraType = "Saturn";
  } else if (keys[GLFW_KEY_7]) {
    cameraType = "Uranus";
  } else if (keys[GLFW_KEY_8]) {
    cameraType = "Neptune";
  } else if (keys[GLFW_KEY_0]) {
    cameraType = "";
  } else if (keys[GLFW_KEY_U]) {
    cameraType = "Up";
  }

  if (keys[GLFW_KEY_M]) {
    speed = speed + 0.1f;
    std::cout << "SPEED : " << speed << std::endl;
  } else if (keys[GLFW_KEY_N]) {
    speed = speed - 0.1f;
    std::cout << "SPEED : " << speed << std::endl;
  }
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
