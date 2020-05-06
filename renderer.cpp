#include "renderer.h"

#include <iostream>

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
	windowState->mouseButton = button;
	windowState->mouseButtonState = action;
    windowState->camera->MouseDown(button, action, mods, (int)windowState->mouseX, (int)windowState->mouseY);
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
    windowState->camera->MouseWheel(xoffset, yoffset);
    windowState->camera->Update();
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
	WindowState *windowState = static_cast<WindowState *>(glfwGetWindowUserPointer(window));
	double xpos,ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
	if( windowState->mouseButtonState == GLFW_PRESS )
	{
		windowState->camera->MouseMove(xpos, ypos);
		windowState->camera->Update();
	}

	windowState->mouseX = xpos;
	windowState->mouseY = ypos;
}

GLRenderer::GLRenderer()
{
    engine = nullptr;
    stage = nullptr;
    window = nullptr;

    this->camera.SetEye(&this->eye);
	this->camera.SetViewMatrix(&this->viewMatrix);
}

GLRenderer::~GLRenderer()
{
}

void GLRenderer::SetSceneBounds(glm::vec3 &sceneMin, glm::vec3 &sceneMax)
{
    this->sceneBounds[0]=sceneMin;
    this->sceneBounds[1]=sceneMax;

    auto &camera = this->GetCamera();
    auto d = sceneMax-sceneMin;
    auto c = sceneMin + (0.5f * d);
    camera.SetTarget(c.xyz());
    camera.SetPosition(glm::vec3(c.x, c.y, c.z - glm::length(d)));

    auto bounds_size = glm::length(d);

    auto extent = this->GetExtent();
    auto &projection = this->GetProjectionMatrix();
    projection = glm::perspective(glm::radians(45.f), (float)extent.x / (float)extent.y, bounds_size/10.f, bounds_size*10.0f);
}

void GLRenderer::CreateWindow(uint32_t width, uint32_t height)
{
    // parameters for the GL renderer
    this->renderParams.renderResolution = pxr::GfVec2i(width, height);
    this->renderParams.showRender = true;
    this->renderParams.enableLighting = true;
    this->renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    this->renderParams.enableSceneMaterials = true;
    this->renderParams.enableUsdDrawModes = true;
    this->renderParams.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_NOTHING;
    this->renderParams.colorCorrectionMode = pxr::HdxColorCorrectionTokens->disabled;

    // initialize glfw and create 4.5 core profile context
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    this->window = glfwCreateWindow(renderParams.renderResolution[0], renderParams.renderResolution[1], "GL Renderer", nullptr, nullptr);

    glfwMakeContextCurrent(window);

    glClearColor(0.15f, 0.15f, 0.95f, 1.f);

    // set some initial params for the camera
    this->camera.SetTarget(glm::vec3(0.f, 0.f, 0.f));
    this->camera.SetPosition(glm::vec3(0.f, 0.f, -1.f));
    this->camera.SetScreenDimensions(glm::vec4(0.f, 0.f, renderParams.renderResolution[0], renderParams.renderResolution[1]));
}

void GLRenderer::BeginRender()
{
    WindowState wstate;
    wstate.camera = &(this->camera);
    glfwSetWindowUserPointer(window, (void *)&wstate);

    // set glfw input callbacks
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);

    engine = new pxr::UsdImagingGLEngine();

    // create the basic light material
    pxr::GlfSimpleMaterial material;
    material.SetAmbient (pxr::GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
    material.SetDiffuse (pxr::GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
    material.SetSpecular(pxr::GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
    material.SetEmission(pxr::GfVec4f(1.0f, 1.0f, 1.0f, 1.0f));
    material.SetShininess(0.15f);

    // setup a couple of lights at 10x the scene min/max
    pxr::GlfSimpleLightVector lights;
    lights.reserve(2);
    pxr::GlfSimpleLight light0;
    auto light0Position = this->sceneBounds[0]*10.f;
    light0.SetPosition(pxr::GfVec4f(light0Position.x, light0Position.y, light0Position.z, 1.f));
    light0.SetAmbient( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light0.SetDiffuse( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light0.SetSpecular(pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    lights.push_back(light0);
    pxr::GlfSimpleLight light1;
    auto light1Position = this->sceneBounds[1]*10.f;
    light1.SetPosition(pxr::GfVec4f(light1Position.x, light1Position.y, light1Position.z, 1.f));
    light1.SetAmbient( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light1.SetDiffuse( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light1.SetSpecular(pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    lights.push_back(light1);
    engine->SetLightingState(lights, material, pxr::GfVec4f(0.15f));

    // load the scene
    auto prim = stage->Load();

    this->camera.SetPosition(sceneBounds[1] * 4.f);
    this->camera.Update();

    // convert from glm to pxr::GfMatrix4d
    auto makeMatrix = [](glm::mat4x4 &mat) -> pxr::GfMatrix4d
    {
        float *arr = glm::value_ptr(mat);
        return pxr::GfMatrix4d( static_cast<double>(arr[ 0]), static_cast<double>(arr[ 1]), static_cast<double>(arr[ 2]), static_cast<double>(arr[ 3]),
                                static_cast<double>(arr[ 4]), static_cast<double>(arr[ 5]), static_cast<double>(arr[ 6]), static_cast<double>(arr[ 7]),
                                static_cast<double>(arr[ 8]), static_cast<double>(arr[ 9]), static_cast<double>(arr[10]), static_cast<double>(arr[11]),
                                static_cast<double>(arr[12]), static_cast<double>(arr[13]), static_cast<double>(arr[14]), static_cast<double>(arr[15]) );
    };

    // setup OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    // render loop
    while( !glfwWindowShouldClose(window) )
    {
        glViewport(0, 0, renderParams.renderResolution[0], renderParams.renderResolution[1]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        engine->SetCameraState(makeMatrix(this->viewMatrix), makeMatrix(this->projectionMatrix));
        engine->Render(prim, this->renderParams);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // cleanup
    if( engine )	
        delete engine;
    engine = nullptr;

    if( window )
        glfwDestroyWindow(window);
    window = nullptr;

    glfwTerminate();
}