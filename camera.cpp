#include "camera.h"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#ifndef GLFW_PRESS
#define GLFW_PRESS 1
#endif

#ifndef GLFW_MOUSE_BUTTON_RIGHT
#define GLFW_MOUSE_BUTTON_RIGHT 1
#endif

Camera::Camera()
    :
    rotateSpeed(1.f),
    zoomSpeed(1.2f),
    dampingFactor(0.2f),
    minDistance(0.f),
    maxDistance(std::numeric_limits<float>::infinity()),
    target(0.f),
    lastPos(0.f),
    eye(0.f),
    rotStart(0.f),
    rotEnd(0.f),
    zoom(0.f),
    panStart(0.f),
    panEnd(0.f),
    panSpeed(0.1f),
    position(0.f, 0.f, 1.f),
    up(0.f,1.f,0.f),
    state(CAMERA_STATE::NONE)
{}

void Camera::Update()
{
    eye = position - target;
    if (state == CAMERA_STATE::ROTATE)
        RotateCamera();
    else if (state == CAMERA_STATE::PAN)
        PanCamera();
    else
        ZoomCamera();
    
    position = target + eye;

    if (glm::length2(position) > maxDistance * maxDistance)
        position = glm::normalize(position) * maxDistance;

    if (glm::length2(eye) < minDistance * minDistance)
    {
        eye = glm::normalize(eye) * minDistance;  
        position = target + eye;
    }

    (*viewMatr) = glm::lookAt(position, target, up);
    (*eyePos) = glm::vec4(position, 1.0);

    if (glm::length2(lastPos - position) > 0.f)
        lastPos = position;
}

void Camera::RotateCamera()
{
    float angle = (float)acos( glm::dot(rotStart, rotEnd) / glm::length(rotStart) / glm::length(rotEnd) );

    if( !std::isnan(angle) && angle != 0.f )
    {
        glm::vec3 axis = glm::normalize(glm::cross( rotStart, rotEnd ));
        if(glm::isnan(axis.x) || glm::isnan(axis.y) || glm::isnan(axis.z))
            return;

        glm::quat quaternion;

        angle *= rotateSpeed;

        quaternion = glm::angleAxis(-angle,axis);
        eye = glm::rotate( quaternion ,  eye);

        up =  glm::rotate(quaternion , up);
        rotEnd =glm::rotate( quaternion , rotEnd);

        quaternion = glm::angleAxis( angle * (dampingFactor - 1.f),axis);
        rotStart = glm::rotate(quaternion,rotStart);
    }
}

void Camera::ZoomCamera()
{
    float factor = 1.f + (float)(-zoom) * zoomSpeed;
    if (factor != 1.f && factor > 0.f)
    {
        eye = eye * (float)factor;
        zoom += (float)(-zoom) * dampingFactor;
    }
}

void Camera::PanCamera()
{
    glm::vec2 mouseChange = panEnd - panStart;

    if (glm::length(mouseChange) != 0.f)
    {
        mouseChange *= glm::length(eye) * panSpeed;
        glm::vec3 pan =glm::normalize(glm::cross(eye, up));

        pan *= mouseChange.x;
        glm::vec3 camUpClone = glm::normalize(up);

        camUpClone *=  mouseChange.y;
        pan += camUpClone;
        position += pan;   

        target +=  pan;
        panStart += (panEnd - panStart) * dampingFactor;
    }
}

glm::vec3 Camera::GetMouseProjectionOnTrackBall(int clientX, int clientY)
{
    auto mouseOnBall = glm::vec3(
        ((float)clientX - (float)screenDimensions.z * 0.5f) / (float)(screenDimensions.z * 0.5f),
        ((float)clientY - (float)screenDimensions.w * 0.5f) / (float)(screenDimensions.w * 0.5f),
        0.f );

    double length = (double)glm::length(mouseOnBall);

    if (length > 1.0)
        mouseOnBall = glm::normalize(mouseOnBall); 
    else
        mouseOnBall.z = (float)sqrt(1.0 - length * length);

    eye = target -  position;    

    glm::vec3 upClone = up;
    upClone = glm::normalize(upClone);
    glm::vec3 projection = upClone * mouseOnBall.y;

    glm::vec3 cross = glm::normalize( glm::cross(up,eye));

    cross *= mouseOnBall.x; 
    projection +=cross;

    glm::vec3 eyeClone = glm::normalize(eye);

    projection += eyeClone * mouseOnBall.z; 

    return projection;
}

void Camera::MouseDown(int button, int action, int mods, int xpos, int ypos)
{
    if( action == GLFW_PRESS )
    {
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
            state = CAMERA_STATE::PAN;
        else
            state = CAMERA_STATE::ROTATE;
    }else
        state = CAMERA_STATE::NONE;

    if (state == CAMERA_STATE::ROTATE)
    {
        rotStart = GetMouseProjectionOnTrackBall(xpos, ypos);
        rotEnd = rotStart;
    }
    else if (state == CAMERA_STATE::PAN)
    {
        panStart = GetMouseOnScreen(xpos, ypos);
        panEnd = panStart;
    }
}

void Camera::MouseWheel(double xoffset ,double yoffset)
{
    if (yoffset != 0.0)
    {
        yoffset /= 3.0;
        zoom += (float)yoffset * 0.05f;
    }
}

void Camera::MouseMove(int xpos,int ypos)
{
    if( state == CAMERA_STATE::ROTATE )
        rotEnd = GetMouseProjectionOnTrackBall(xpos,ypos);
    else if( state == CAMERA_STATE::PAN )
        panEnd = GetMouseOnScreen(xpos,ypos);
}

glm::vec2 Camera::GetMouseOnScreen(int clientX, int clientY)
{
	return glm::vec2(
		(float)(clientX - screenDimensions.x) / screenDimensions.z,
		(float)(clientY - screenDimensions.y) / screenDimensions.w );
}

void Camera::MouseUp()
{
	state = CAMERA_STATE::NONE;
}
