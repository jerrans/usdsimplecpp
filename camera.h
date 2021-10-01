#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

class Camera
{
public:
	Camera();
	virtual ~Camera(){}

	virtual void SetEye(glm::vec4 *eye) { eyePos = eye; }
	virtual void SetViewMatrix(glm::mat4 *viewMatrix) { viewMatr = viewMatrix; }
	
    virtual void SetPosition(glm::vec3 cameraPosition) { position = cameraPosition; }
	virtual glm::vec3 GetPosition() { return position; }

	virtual void SetTarget(glm::vec3 cameraTarget) { target = cameraTarget; }
	virtual glm::vec3 GetTarget() { return target; }
	
    virtual void SetScreenDimensions(glm::vec4 screenDims) { screenDimensions = screenDims; }
	virtual glm::vec4 GetScreenDimensions() { return screenDimensions; }
	virtual glm::mat4 *GetViewMatrix() { return viewMatr; }
	glm::vec2 GetMouseOnScreen(int clientX, int clientY);

	virtual void Update();
	virtual void MouseUp();
	virtual void MouseDown(int button, int action, int mods,int xpos,int ypos);
	virtual void MouseMove(int xpos, int ypos);
	virtual void MouseWheel(double xoffset ,double yoffset);

protected:
	glm::vec3 GetMouseProjectionOnTrackBall(int clientX, int clientY);
	virtual void RotateCamera();
	virtual void ZoomCamera();
	virtual void PanCamera();

	enum CAMERA_STATE{
		NONE=0,
		ROTATE,
		PAN
	};

	glm::mat4 *viewMatr;
	glm::vec4 screenDimensions;

	glm::vec3 target, eye, lastPos, rotStart, rotEnd, up, position;
	glm::vec4 *eyePos;
	glm::vec2 panStart, panEnd;

    float rotateSpeed;
	float zoomSpeed;
	float panSpeed;

	float minDistance;
	float maxDistance;

	float dampingFactor;
	float zoom;

	CAMERA_STATE state;
};