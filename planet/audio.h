#include <iostream>
#define MINIAUDIO_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <stdio.h>

using namespace std;
extern GLFWwindow *window;
extern float volume;
extern const char *songs[];
extern float deltaTime;
extern bool shouldSkip;
void initializeMiniaudio();
