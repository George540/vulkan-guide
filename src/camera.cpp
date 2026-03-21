#include "camera.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::update()
{
    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.0f));
}

void Camera::processSDLEvent(SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN)
    {
        if (event.key.keysym.sym == SDLK_w) { velocity.z = -1; }
        if (event.key.keysym.sym == SDLK_s) { velocity.z = 1; }
        if (event.key.keysym.sym == SDLK_a) { velocity.x = -1; }
        if (event.key.keysym.sym == SDLK_d) { velocity.x = 1; }
    }

    if (event.type == SDL_KEYUP)
    {
        if (event.key.keysym.sym == SDLK_w) { velocity.z = 0; }
        if (event.key.keysym.sym == SDLK_s) { velocity.z = 0; }
        if (event.key.keysym.sym == SDLK_a) { velocity.x = 0; }
        if (event.key.keysym.sym == SDLK_d) { velocity.x = 0; }
    }

    if (event.type == SDL_MOUSEMOTION)
    {
        yaw += (float)event.motion.xrel / 200.f;
        pitch -= (float)event.motion.yrel / 200.f;
    }
}

glm::mat4 Camera::getViewMatrix()
{
    // To create a correct model view, we need to move the world in opposite direction to the camera
    // so we will create the camera model matrix and invert.
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix()
{
    // Fairly typical FPS style camera. we join the pitch and yaw rotations into the final rotation matrix
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.0f, 0.0f, 0.0f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.0f, -1.0f, 0.0f });

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

