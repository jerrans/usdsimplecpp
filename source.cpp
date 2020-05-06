#include "renderer.h"

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdHydra/tokens.h>

#include <iostream>

pxr::UsdGeomMesh createMesh(pxr::UsdStageRefPtr stage, const std::string &meshName, pxr::VtVec3fArray &points, pxr::VtArray<int> &faceVertexCounts, pxr::VtArray<int> &faceVertexIndices)
{
    // find the geometric extents of the mesh
    pxr::VtVec3fArray extent(2);
    extent[0] = points[0];
    extent[1] = points[0];
    for( auto &pt : points )
    {
        for( int i=0; i<3; ++i )
        {
            extent[0][i]=std::min(pt[0], extent[0][i]);
            extent[1][i]=std::max(pt[0], extent[1][i]);
        }
    }

    // create the mesh and set its points, face vertex counts (number of vertices for each face) and face vertex indices
    auto mesh = pxr::UsdGeomMesh::Define(stage, pxr::SdfPath("/" + meshName));

    mesh.GetPointsAttr().Set(points);
    mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts);
    mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices);

    // set geometric extents
    mesh.GetExtentAttr().Set(extent);

    mesh.GetDoubleSidedAttr().Set(true);

    return mesh;
}

pxr::UsdGeomMesh createMesh(pxr::UsdStageRefPtr stage, const std::string &primName, pxr::VtVec3fArray &points, pxr::VtArray<int> &faceVertexCounts, pxr::VtArray<int> &faceVertexIndices, pxr::VtVec2fArray &texCoordArray)
{
    auto mesh = createMesh(stage, primName, points, faceVertexCounts, faceVertexIndices);

    // add a new primvar for texture coordinates that we can reference in the shader
    auto texCoords = mesh.CreatePrimvar(pxr::TfToken("st"), pxr::SdfValueTypeNames->TexCoord2fArray, pxr::UsdGeomTokens->varying);

    // set the texture coords
    texCoords.Set(texCoordArray);

    return mesh;
}

pxr::UsdShadeShader createPBRShader(pxr::UsdStageRefPtr stage, pxr::UsdGeomMesh &mesh, const float roughness, const float metallic, const std::string &textureFile)
{
    // create path hierarchy
    auto path = mesh.GetPrim().GetPrimPath();
    auto materialPath = path.AppendPath(pxr::SdfPath("material"));
    auto pbrShaderPath = materialPath.AppendPath(pxr::SdfPath("PBRShader"));
    
    auto material = pxr::UsdShadeMaterial::Define(stage, materialPath);
    
    // create the basic PBR shader and set params
    auto pbrShader = pxr::UsdShadeShader::Define(stage, pbrShaderPath);
    pbrShader.CreateIdAttr(pxr::VtValue(pxr::TfToken("UsdPreviewSurface")));
    pbrShader.CreateInput(pxr::TfToken("roughness"), pxr::SdfValueTypeNames->Float).Set(roughness);
    pbrShader.CreateInput(pxr::TfToken("metallic"), pxr::SdfValueTypeNames->Float).Set(metallic);

    // connect the pbr shader to the material
    material.CreateSurfaceOutput().ConnectToSource(pbrShader, pxr::TfToken("surface"));

    // bind material to the mesh
    pxr::UsdShadeMaterialBindingAPI(mesh).Bind(material);

    if( textureFile.empty() ) // texturing?
    {
        // if not then just set a red material
        auto clr = pxr::GfVec3f(1.0f, 0.0f, 0.0f);
        pbrShader.CreateInput(pxr::TfToken("diffuseColor"), pxr::SdfValueTypeNames->Color3f).Set(clr);
    }else{
        // first create the reader
        auto stReaderPath = materialPath.AppendPath(pxr::SdfPath("stReader"));
        auto stReader = pxr::UsdShadeShader::Define(stage, stReaderPath);
        stReader.CreateIdAttr(pxr::VtValue(pxr::TfToken("UsdPrimvarReader_float2")));

        // create the texture sampler
        auto diffuseTextureSamplerPath = materialPath.AppendPath(pxr::SdfPath("diffuseTexture"));
        auto diffuseTextureSampler = pxr::UsdShadeShader::Define(stage, diffuseTextureSamplerPath);
        diffuseTextureSampler.CreateIdAttr(pxr::VtValue(pxr::TfToken("UsdUVTexture")));
        diffuseTextureSampler.CreateInput(pxr::TfToken("file"), pxr::SdfValueTypeNames->Asset).Set(pxr::SdfAssetPath(textureFile));
        diffuseTextureSampler.CreateInput(pxr::TfToken("st"), pxr::SdfValueTypeNames->Float2).ConnectToSource(stReader, pxr::TfToken("result"));

        // this bit is important...by default it will use LINEAR_MIPMAP_LINEAR (for some reason usdview doesn't require this though), thanks RenderDoc :)
        diffuseTextureSampler.CreateInput(pxr::UsdHydraTokens->minFilter, pxr::SdfValueTypeNames->Token).Set(pxr::UsdHydraTokens->linear);

        // attach the output of the sampler to the pbr shader's diffuseColor
        diffuseTextureSampler.CreateOutput(pxr::TfToken("rgb"), pxr::SdfValueTypeNames->Float3);
        pbrShader.CreateInput(pxr::TfToken("diffuseColor"), pxr::SdfValueTypeNames->Color3f).ConnectToSource(diffuseTextureSampler, pxr::TfToken("rgb"));

        // connect everything together
        auto stInput = material.CreateInput(pxr::TfToken("frame:stPrimvarName"), pxr::SdfValueTypeNames->Token);
        stInput.Set(pxr::TfToken("st"));

        stReader.CreateInput(pxr::TfToken("varname"), pxr::SdfValueTypeNames->Token).ConnectToSource(stInput);
    }

    return pbrShader;
}

pxr::SdfLayerRefPtr cube(const std::string &primName, const std::string textureFile)
{
    // indices for cube triangles
    pxr::VtArray<int> faceIndices = {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    // all faces are triangles
    pxr::VtArray<int> faceIndexCounts = {
        3,3,3,3,3,3,3,3,3,3,3,3
    };
    
    // 24 points for a cube? well yes because they have distinct normals
    pxr::VtVec3fArray cube(24);
    cube[ 0] = pxr::GfVec3f( 1.f, -1.f, -1.f);
    cube[ 1] = pxr::GfVec3f( 1.f, -1.f,  1.f);
    cube[ 2] = pxr::GfVec3f(-1.f, -1.f,  1.f);
    cube[ 3] = pxr::GfVec3f(-1.f, -1.f, -1.f);
    cube[ 4] = pxr::GfVec3f( 1.f,  1.f, -1.f);
    cube[ 5] = pxr::GfVec3f(-1.f,  1.f, -1.f);
    cube[ 6] = pxr::GfVec3f(-1.f,  1.f,  1.f);
    cube[ 7] = pxr::GfVec3f( 1.f,  1.f,  1.f);
    cube[ 8] = pxr::GfVec3f( 1.f, -1.f, -1.f);
    cube[ 9] = pxr::GfVec3f( 1.f,  1.f, -1.f);
    cube[10] = pxr::GfVec3f( 1.f,  1.f,  1.f);
    cube[11] = pxr::GfVec3f( 1.f, -1.f,  1.f);
    cube[12] = pxr::GfVec3f( 1.f, -1.f,  1.f);
    cube[13] = pxr::GfVec3f( 1.f,  1.f,  1.f);
    cube[14] = pxr::GfVec3f(-1.f,  1.f,  1.f);
    cube[15] = pxr::GfVec3f(-1.f, -1.f,  1.f);
    cube[16] = pxr::GfVec3f(-1.f, -1.f,  1.f);
    cube[17] = pxr::GfVec3f(-1.f,  1.f,  1.f);
    cube[18] = pxr::GfVec3f(-1.f,  1.f, -1.f);
    cube[19] = pxr::GfVec3f(-1.f, -1.f, -1.f);
    cube[20] = pxr::GfVec3f( 1.f,  1.f, -1.f);
    cube[21] = pxr::GfVec3f( 1.f, -1.f, -1.f);
    cube[22] = pxr::GfVec3f(-1.f, -1.f, -1.f);
    cube[23] = pxr::GfVec3f(-1.f,  1.f, -1.f);

    // tex coords...if a texture was specified we'll need these
    pxr::VtVec2fArray texCoords(24);
    texCoords[ 0] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[ 1] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[ 2] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[ 3] = pxr::GfVec2f( 0.f,  1.f);
    texCoords[ 4] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[ 5] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[ 6] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[ 7] = pxr::GfVec2f( 0.f,  1.f);
    texCoords[ 8] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[ 9] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[10] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[11] = pxr::GfVec2f( 0.f,  1.f);
    texCoords[12] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[13] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[14] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[15] = pxr::GfVec2f( 0.f,  1.f);
    texCoords[16] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[17] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[18] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[19] = pxr::GfVec2f( 0.f,  1.f);
    texCoords[20] = pxr::GfVec2f( 0.f,  0.f);
    texCoords[21] = pxr::GfVec2f( 1.f,  0.f);
    texCoords[22] = pxr::GfVec2f( 1.f,  1.f);
    texCoords[23] = pxr::GfVec2f( 0.f,  1.f);

    // create an anonymous layer in which to create the geometry
    auto layer = pxr::SdfLayer::CreateAnonymous(primName + ".usda");
    auto stage = pxr::UsdStage::Open(layer);

    auto mesh = createMesh(stage, primName, cube, faceIndexCounts, faceIndices, texCoords);
    
    auto pbrShader = createPBRShader(stage, mesh, 0.4f, 0.f, textureFile);

    return layer;
}

pxr::SdfLayerRefPtr cube(const std::string &primName)
{
    return cube(primName, "");
}

int main(int argc, char **argv)
{
    if( argc == 2 )
    {
        std::cout << "Using specified texture filename: " << argv[1] << std::endl;
    }

    GLRenderer renderer;

    auto usdStage = pxr::UsdStage::CreateNew("helloWorld.usda");
    
    // create cube geometry and material on anonymous layer
    std::string primName("cube");
    auto cubeLayer = argc != 2 ? cube(primName) : cube(primName, argv[1]);

    // transfer content to the root layer of the stage
    usdStage->GetRootLayer()->TransferContent(cubeLayer);

    // get the extents of the geometry
    auto prim = usdStage->GetPrimAtPath(pxr::SdfPath("/" + primName));
    if( auto extentAttr = prim.GetAttribute(pxr::UsdGeomTokens->extent) )
    {
        pxr::VtVec3fArray extentArray(2);
        extentAttr.Get(&extentArray);
        glm::vec3 gmin(extentArray[0][0], extentArray[0][1], extentArray[0][2]);
        glm::vec3 gmax(extentArray[1][0], extentArray[1][1], extentArray[1][2]);
        renderer.SetSceneBounds(gmin, gmax);
    }
    
    renderer.SetUsdStage(usdStage);
    renderer.CreateWindow(800, 600);
    renderer.BeginRender();

    // save stage to file
    usdStage->Save();
    return 0;
}
