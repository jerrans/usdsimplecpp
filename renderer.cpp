#include <GL/glew.h>
#include "renderer.h"
#include "shader.h"

#include <iostream>

#define PRIMARY_DEPTH_VIS 0
#define SECONDARY_DEPTH_VIS 0

std::string s_vs =
"#version 410\n"
"layout(location = 0) out vec2 uv;\n"
"void main()\n"
"{\n"
"    float x = float(((uint(gl_VertexID) + 2u) / 3u) % 2u);\n"
"    float y = float(((uint(gl_VertexID) + 1u) / 3u) % 2u);\n"
"\n"
"    gl_Position = vec4(-1.0f + x * 2.0f, -1.0f + y * 2.0f, 0.0f, 1.0f);\n"
"    uv = vec2(x, y);\n"
"}\n";

std::string s_fs =
"#version 410\n"
"layout(location = 0) in vec2 uv;\n"
"uniform sampler2D primary;"
"uniform sampler2D secondary;"
"out vec4 fragColor;"
"void main()\n"
"{\n"
"   if(uv.y > 0.5){\n"
#if SECONDARY_DEPTH_VIS
"       float d = texture(secondary, uv).r;\n"
"       d = (d + 1.0) / 2.0;\n"
"       fragColor = vec4(d, d, d, 1.0);\n"
#else
"       fragColor = texture(secondary, uv).rgba;\n"
#endif
"   }else{\n"
#if PRIMARY_DEPTH_VIS
"       float d = texture(primary, uv).r;\n"
"       fragColor = vec4(d, d, d, 1.0);\n"
#else
"       fragColor = texture(primary, uv).rgba;\n"
#endif
"   }\n"
"   vec4 overlay = texture(secondary, uv).rgba;\n"
"   fragColor = overlay.r >= 1.0 ? texture(primary, uv).rgba : texture(secondary, uv).rgba;\n"
"}\n";

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

void window_size_callback(GLFWwindow* window, int width, int height)
{
    WindowState* windowState = static_cast<WindowState*>(glfwGetWindowUserPointer(window));
    windowState->camera->SetScreenDimensions(glm::vec4(0.f, 0.f, (float)width, (float)height));
}

GLRenderer::GLRenderer()
{
    primaryGraphicsEngine = nullptr;
    secondaryGraphicsEngine = nullptr;
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

void GLRenderer::CreateGLWindow(uint32_t width, uint32_t height)
{
    // load the scene
    //auto prim = stage->Load();
    //stage = pxr::UsdStage::Open("c:\\src\\datasets\\flighthelmet.usdc");
    //stage = pxr::UsdStage::Open("c:\\src\\datasets\\Kitchen_set\\assets\\WoodenDryingRack\\WoodenDryingRack.geom.usd");
    stage = pxr::UsdStage::Open("c:\\src\\datasets\\Kitchen_set\\Kitchen_set.usd");
    auto range = stage->Traverse();
    bool extentsSet = false;
    glm::vec3 extentMin, extentMax;
    for (auto &it = range.begin(); it != range.end(); it++)
    {
        if (auto extentAttr = (*it).GetAttribute(pxr::UsdGeomTokens->extent))
        {
            pxr::VtVec3fArray extentArray(2);
            extentAttr.Get(&extentArray);
            glm::vec3 gmin(extentArray[0][0], extentArray[0][1], extentArray[0][2]);
            glm::vec3 gmax(extentArray[1][0], extentArray[1][1], extentArray[1][2]);

            if (!extentsSet)
            {
                extentMin = gmin;
                extentMax = gmax;
                extentsSet = true;
            }
            else {
                extentMin.x = std::min(extentMin.x, gmin.x);
                extentMin.y = std::min(extentMin.y, gmin.y);
                extentMin.z = std::min(extentMin.z, gmin.z);
                extentMax.x = std::max(extentMax.x, gmax.x);
                extentMax.y = std::max(extentMax.y, gmax.y);
                extentMax.z = std::max(extentMax.z, gmax.z);
            }
        }
    }

    if(extentsSet)
        this->SetSceneBounds(extentMin, extentMax);

    /*
    std::copy_if(range.begin(), range.end(), std::back_inserter(meshes),
              [](UsdPrim const &) { return prim.IsA<UsdGeomMesh>(); });*/
 /*
    if (auto extentAttr = prim.GetAttribute(pxr::UsdGeomTokens->extent))
    {
        pxr::VtVec3fArray extentArray(2);
        extentAttr.Get(&extentArray);
        glm::vec3 gmin(extentArray[0][0], extentArray[0][1], extentArray[0][2]);
        glm::vec3 gmax(extentArray[1][0], extentArray[1][1], extentArray[1][2]);
        this->SetSceneBounds(gmin, gmax);
    }
    */
    std::cout << "Renderer Plugins: " << std::endl;
    const auto plugins = pxr::UsdImagingGLEngine::GetRendererPlugins();
    int rendererPluginCount = 0;
    for (const auto& token : plugins) {
        const auto name = token.GetString();
        std::cout << ++rendererPluginCount << ". " << name << std::endl;
        rendererPlugins[rendererPluginCount] = token;
        if( name == "HdStormRendererPlugin")
            activeRendererPlugin = token;
    }

    int pluginIndex = 0;
    std::cout << "Renderer Plugin: ";
    std::cin >> pluginIndex;
    if (rendererPlugins.find(pluginIndex) != rendererPlugins.end())
        activeRendererPlugin = rendererPlugins[pluginIndex];

    // parameters for the GL renderer
    this->renderParams.showRender = true;
    this->renderParams.enableLighting = true;
    this->renderParams.drawMode = pxr::UsdImagingGLDrawMode::DRAW_SHADED_SMOOTH;
    this->renderParams.enableSceneMaterials = true;
    this->renderParams.enableUsdDrawModes = true;
    this->renderParams.cullStyle = pxr::UsdImagingGLCullStyle::CULL_STYLE_NOTHING;
    this->renderParams.colorCorrectionMode = pxr::HdxColorCorrectionTokens->disabled;
    this->renderParams.clearColor = pxr::GfVec4f(17.f / 255.f, 80.f / 255.f, 147.f / 255.f, 1.f);

    // initialize glfw and create 4.5 core profile context
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    this->window = glfwCreateWindow(width, height, "GL Renderer", nullptr, nullptr);

    glfwMakeContextCurrent(window);
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }

    glClearColor(0.15f, 0.15f, 0.95f, 1.f);

    // set some initial params for the camera
    this->camera.SetTarget(glm::vec3(0.f, 0.f, 0.f));
    this->camera.SetPosition(glm::vec3(0.f, 0.f, -1.f));
    this->camera.SetScreenDimensions(glm::vec4(0.f, 0.f, (float)width, (float)height));

    quadShader = std::make_shared<Shader>();
    quadShader->SetShaderSource(s_vs, "", s_fs, {}, {"fragColor"});
    if (quadShader->CompileLink() == false)
        throw std::runtime_error("Failed to compile shader programs.");
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
    glfwSetWindowSizeCallback(window, window_size_callback);

    primaryGraphicsEngine = new pxr::UsdImagingGLEngine();
    primaryGraphicsEngine->SetRendererPlugin(rendererPlugins[1]);
    //primaryGraphicsEngine->SetRendererPlugin(activeRendererPlugin);

    secondaryGraphicsEngine = new pxr::UsdImagingGLEngine();
    secondaryGraphicsEngine->SetRendererPlugin(rendererPlugins[3]);

    static pxr::TfToken tokenDenoisingEnabled("OxideDenoiseEnabled");
    primaryGraphicsEngine->SetRendererSetting(tokenDenoisingEnabled, pxr::VtValue(false));

    // create the basic light material
    pxr::GlfSimpleMaterial material;
    material.SetAmbient (pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    material.SetDiffuse (pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    material.SetSpecular(pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    material.SetEmission(pxr::GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    material.SetShininess(0.15f);

    // setup a couple of lights at 10x the scene min/max
    pxr::GlfSimpleLightVector lights;
    lights.reserve(2);
    pxr::GlfSimpleLight light0;
    auto light0Position = this->sceneBounds[0]*2.f;
    light0.SetPosition(pxr::GfVec4f(light0Position.x, light0Position.y, light0Position.z, 1.f));
    light0.SetAmbient( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light0.SetDiffuse( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light0.SetSpecular(pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    lights.push_back(light0);
    pxr::GlfSimpleLight light1;
    auto light1Position = this->sceneBounds[1]*2.f;
    light1.SetPosition(pxr::GfVec4f(light1Position.x, light1Position.y, light1Position.z, 1.f));
    light1.SetAmbient( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light1.SetDiffuse( pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    light1.SetSpecular(pxr::GfVec4f(1.f, 1.f, 1.f, 1.f));
    lights.push_back(light1);

    primaryGraphicsEngine->SetLightingState(lights, material, pxr::GfVec4f(0.15f));
    secondaryGraphicsEngine->SetLightingState(lights, material, pxr::GfVec4f(0.15f));

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
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA);
    GLuint emptyVAO;
    glGenVertexArrays(1, &emptyVAO);
    
    // render loop
    while( !glfwWindowShouldClose(window) )
    {
        auto screenDims = this->camera.GetScreenDimensions();
        glm::ivec2 windowDims((uint32_t)screenDims.z, (uint32_t)screenDims.w);
        primaryGraphicsEngine->SetCameraState(makeMatrix(this->viewMatrix), makeMatrix(this->projectionMatrix));
        primaryGraphicsEngine->SetRenderBufferSize(pxr::GfVec2i(windowDims.x, windowDims.y));
        primaryGraphicsEngine->SetRendererAov(pxr::HdAovTokens->color);
        primaryGraphicsEngine->SetRenderViewport(pxr::GfVec4d(0, 0, windowDims.x, windowDims.y));
        primaryGraphicsEngine->SetWindowPolicy(pxr::CameraUtilConformWindowPolicy::CameraUtilFit);
        primaryGraphicsEngine->Render(stage->GetPseudoRoot(), this->renderParams);
        
        secondaryGraphicsEngine->SetCameraState(makeMatrix(this->viewMatrix), makeMatrix(this->projectionMatrix));
        secondaryGraphicsEngine->SetRenderBufferSize(pxr::GfVec2i(windowDims.x, windowDims.y));
        secondaryGraphicsEngine->SetRendererAov(pxr::HdAovTokens->color);
        secondaryGraphicsEngine->SetRenderViewport(pxr::GfVec4d(0, 0, windowDims.x, windowDims.y));
        secondaryGraphicsEngine->SetWindowPolicy(pxr::CameraUtilConformWindowPolicy::CameraUtilFit);

        auto depthTexture = primaryGraphicsEngine->GetAovTexture(pxr::HdAovTokens->depth);
        if (depthTexture)
        {
            secondaryGraphicsEngine->PopulateAovTexture(pxr::HdAovTokens->depth, depthTexture);
        }
        secondaryGraphicsEngine->Render(stage->GetPseudoRoot(), this->renderParams);

        glViewport(0, 0, windowDims.x, windowDims.y);
        glClearColor(17.f / 255.f, 80.f / 255.f, 147.f / 255.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(emptyVAO);

        glActiveTexture(GL_TEXTURE0);
#if PRIMARY_DEPTH_VIS
        glBindTexture(GL_TEXTURE_2D, primaryGraphicsEngine->GetAovTexture(pxr::HdAovTokens->depth)->GetRawResource());
#else
        glBindTexture(GL_TEXTURE_2D, primaryGraphicsEngine->GetAovTexture(pxr::HdAovTokens->color)->GetRawResource());
#endif
        glActiveTexture(GL_TEXTURE1);

#if SECONDARY_DEPTH_VIS
        glBindTexture(GL_TEXTURE_2D, secondaryGraphicsEngine->GetAovTexture(pxr::HdAovTokens->depth)->GetRawResource());
#else
        glBindTexture(GL_TEXTURE_2D, secondaryGraphicsEngine->GetAovTexture(pxr::HdAovTokens->color)->GetRawResource());
#endif

        quadShader->Activate();
        quadShader->SetUniform("primary", 0);
        quadShader->SetUniform("secondary", 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        quadShader->Deactivate();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // cleanup
    if(primaryGraphicsEngine)
        delete primaryGraphicsEngine;
    primaryGraphicsEngine = nullptr;
    if (secondaryGraphicsEngine)
        delete secondaryGraphicsEngine;
    secondaryGraphicsEngine = nullptr;

    if( window )
        glfwDestroyWindow(window);
    window = nullptr;

    glfwTerminate();
}
