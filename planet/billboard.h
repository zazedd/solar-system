#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

class Billboard {

public:

	glm::vec3 position;
	glm::mat4 modelTransform;

	Billboard(glm::vec3 position);
	void update(glm::vec3 playerPosition);
};