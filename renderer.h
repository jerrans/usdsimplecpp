#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/imaging/hdx/tokens.h>

#include <GLFW/glfw3.h>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h"

class Shader;

// attach this as the user data pointer to the glfw callbacks
struct WindowState
{
	WindowState()
		: mouseX(0.0), mouseY(0.0), mouseButton(-1), mouseButtonState(-1)
	{}
	double mouseX, mouseY;
	int mouseButton;
	int mouseButtonState;
	Camera *camera;
};

class GLRenderer
{
    public:
    GLRenderer();
    virtual ~GLRenderer();

    virtual void CreateGLWindow(uint32_t width, uint32_t height);
    virtual void BeginRender();
    virtual void SetSceneBounds(glm::vec3 &sceneMin, glm::vec3 &sceneMax);

    virtual Camera &GetCamera()
    {
        return this->camera;
    }
    virtual glm::mat4x4 &GetProjectionMatrix()
    {
        return this->projectionMatrix;
    }
    virtual glm::ivec2 GetExtent()
    {
        return glm::ivec2(800, 600);
    }
    void SetUsdStage(pxr::UsdStageRefPtr stg)
    {
        stage = stg;
    }

    protected:
    GLFWwindow* window;

    // Usd
    pxr::UsdStageRefPtr stage;
    pxr::UsdImagingGLEngine *primaryGraphicsEngine;
    pxr::UsdImagingGLEngine *secondaryGraphicsEngine;
    pxr::UsdImagingGLRenderParams primaryRenderParams;
    pxr::UsdImagingGLRenderParams secondaryRenderParams;

    Camera camera;

    glm::mat4x4 projectionMatrix;
    glm::vec4 eye;
    glm::mat4 viewMatrix;
    glm::vec3 sceneBounds[2];

    std::map<int, pxr::TfToken> rendererPlugins;
    pxr::TfToken activeRendererPlugin;

    std::shared_ptr<Shader> quadShader;
};
