#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        this->cameraFrontDirection = glm::normalize(this->cameraTarget - this->cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        //TODO

        return glm::lookAt(this->cameraPosition, this->cameraFrontDirection + this->cameraPosition, this->cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            this->cameraPosition += this->cameraFrontDirection * speed;
        }
        if (direction == MOVE_BACKWARD) {
            this->cameraPosition -= this->cameraFrontDirection * speed;
        }
        if (direction == MOVE_RIGHT) {
            this->cameraPosition += this->cameraRightDirection * speed;
        }
        if (direction == MOVE_LEFT) {
            this->cameraPosition -= this->cameraRightDirection * speed;
        }
        if (direction == MOVE_UP) {
            this->cameraPosition += this->cameraUpDirection * speed;
        }
        if (direction == MOVE_DOWN) {
            this->cameraPosition -= this->cameraUpDirection * speed;
        }
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 direction;

        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

        cameraFrontDirection = glm::normalize(direction);
        cameraTarget = cameraFrontDirection + cameraPosition;
        cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUpDirection));
    }

    glm::vec3 Camera::getCameraPosition() {
        return this->cameraPosition;
    }
}