// *************************************************************************************************
//
// Horde3D
//   Next-Generation Graphics Engine
//
// Sample Application
// --------------------------------------
// Copyright (C) 2006-2011 Nicolas Schulz
//
//
// This sample source file is not covered by the EPL as the rest of the SDK
// and may be used without any restrictions. However, the EPL's disclaimer of
// warranty and liability shall be in effect for this file.
//
// *************************************************************************************************

#include "app.h"
#include "Horde3D.h"
#include "Horde3DUtils.h"
#include <math.h>
#include <iomanip>

using namespace std;

// Convert from degrees to radians
inline float degToRad( float f )
{
    return f * (3.1415926f / 180.0f);
}


Application::Application( const std::string &appPath )
{
    for( unsigned int i = 0; i < 320; ++i )
    {
        _keys[i] = false;
        _prevKeys[i] = false;
    }

    _x = 5; _y = 3; _z = 19; _rx = 7; _ry = 15; _velocity = 10.0f;
    _curFPS = 30;

    _statMode = 0;
    _freezeMode = 0; _debugViewMode = false; _wireframeMode = false;
    _animTime = 0; _weight = 1.0f;
    _cam = 0;

    _contentDir = appPath + "../Content";
}


bool Application::init() {
    // Initialize engine
    if (!h3dInit()) {
        h3dutDumpMessages();
        return false;
    }

    // Set options
    h3dSetOption(H3DOptions::LoadTextures, 1);
    h3dSetOption(H3DOptions::TexCompression, 0);
    h3dSetOption(H3DOptions::FastAnimation, 0);
    h3dSetOption(H3DOptions::MaxAnisotropy, 4);
    h3dSetOption(H3DOptions::ShadowMapSize, 2048);

    // Add resources
    // Pipelines
    _hdrPipeRes = h3dAddResource(H3DResTypes::Pipeline, "pipelines/hdr.pipeline.xml", 0);
    _forwardPipeRes = h3dAddResource(H3DResTypes::Pipeline, "pipelines/forward.pipeline.xml", 0);
    // Overlays
    _fontMatRes = h3dAddResource(H3DResTypes::Material, "overlays/font.material.xml", 0);
    _panelMatRes = h3dAddResource(H3DResTypes::Material, "overlays/panel.material.xml", 0);
    _logoMatRes = h3dAddResource(H3DResTypes::Material, "overlays/logo.material.xml", 0);
    // Environment
    H3DRes envRes = h3dAddResource(H3DResTypes::SceneGraph, "models/sphere/sphere.scene.xml", 0);

    H3DRes panelRes = h3dAddResource(H3DResTypes::SceneGraph, "models/control-panel/control-panel.scene.xml", 0);

    // Skybox
    H3DRes skyBoxRes = h3dAddResource( H3DResTypes::SceneGraph, "models/skybox/skybox.scene.xml", 0 );


    // Load resources
    h3dutLoadResourcesFromDisk(_contentDir.c_str());

    // Add scene nodes
    // Add camera
    _cam = h3dAddCameraNode(H3DRootNode, "Camera", _hdrPipeRes);
    h3dSetNodeTransform(_cam, 0, 5, 8, 330, 0, 0, 1, 1, 1);

    //h3dSetNodeParamI( _cam, H3DCamera::OccCullingI, 1 );
    // Add environment
    H3DNode env = h3dAddNodes(H3DRootNode, envRes);
    h3dSetNodeTransform(env, 0, -20, 0, 0, 0, 0, 20, 20, 20);
    //
    _panel = h3dAddNodes(H3DRootNode, panelRes);
    h3dSetNodeTransform(_panel, 0, 0, 0, 0, 0, 0, 1, 1, 1);

    // Add skybox
    H3DNode sky = h3dAddNodes( H3DRootNode, skyBoxRes );
    h3dSetNodeTransform( sky, 0, 0, 0, 0, 0, 0, 210, 50, 210 );
    h3dSetNodeFlags( sky, H3DNodeFlags::NoCastShadow, true );

    // Add light source
    H3DNode light = h3dAddLightNode(H3DRootNode, "Light1", 0, "LIGHTING", "SHADOWMAP");
    h3dSetNodeTransform(light, 0, 15, 10, -60, 0, 0, 1, 1, 1);
    h3dSetNodeParamF(light, H3DLight::RadiusF, 0, 30);
    h3dSetNodeParamF(light, H3DLight::FovF, 0, 90);
    h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 1);
    h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, 0.01f);
    h3dSetNodeParamF(light, H3DLight::ColorF3, 0, 1.0f);
    h3dSetNodeParamF(light, H3DLight::ColorF3, 1, 0.8f);
    h3dSetNodeParamF(light, H3DLight::ColorF3, 2, 0.7f);
    h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, 1.0f);

    // Customize post processing effects
    H3DNode matRes = h3dFindResource(H3DResTypes::Material, "pipelines/postHDR.material.xml");
    h3dSetMaterialUniform(matRes, "hdrExposure", 2.5f, 0, 0, 0);
    h3dSetMaterialUniform(matRes, "hdrBrightThres", 0.5f, 0, 0, 0);
    h3dSetMaterialUniform(matRes, "hdrBrightOffset", 0.08f, 0, 0, 0);

    return true;
}


void Application::mainLoop( float fps ) {
    _curFPS = fps;

    h3dSetOption(H3DOptions::DebugViewMode, _debugViewMode ? 1.0f : 0.0f);
    h3dSetOption(H3DOptions::WireframeMode, _wireframeMode ? 1.0f : 0.0f);

    // Set camera parameters
    //h3dSetNodeTransform(_cam, _x, _y, _z, _rx, _ry, 0, 1, 1, 1);

    // Show stats
    h3dutShowFrameStats(_fontMatRes, _panelMatRes, _statMode);
    if (_statMode > 0) {
        // Display weight
        _text.str("");
        _text << fixed << setprecision(2) << "Weight: " << _weight;
        h3dutShowText(_text.str().c_str(), 0.03f, 0.24f, 0.026f, 1, 1, 1, _fontMatRes);
    }

    // Show logo
    const float ww = (float) h3dGetNodeParamI(_cam, H3DCamera::ViewportWidthI) /
                     (float) h3dGetNodeParamI(_cam, H3DCamera::ViewportHeightI);
    const float ovLogo[] = {ww - 0.4f, 0.8f, 0, 1, ww - 0.4f, 1, 0, 0, ww, 1, 1, 0, ww, 0.8f, 1, 1};
    h3dShowOverlays(ovLogo, 4, 1.f, 1.f, 1.f, 1.f, _logoMatRes, 0);

    // Render scene
    h3dRender(_cam);

    // Finish rendering of frame
    h3dFinalizeFrame();

    // Remove all overlays
    h3dClearOverlays();

    // Write all messages to log file
    h3dutDumpMessages();
}


void Application::release()
{
    // Release engine
    h3dRelease();
}


void Application::resize( int width, int height ) {
    // Resize viewport
    h3dSetNodeParamI(_cam, H3DCamera::ViewportXI, 0);
    h3dSetNodeParamI(_cam, H3DCamera::ViewportYI, 0);
    h3dSetNodeParamI(_cam, H3DCamera::ViewportWidthI, width);
    h3dSetNodeParamI(_cam, H3DCamera::ViewportHeightI, height);

    // Set virtual camera parameters
    h3dSetupCameraView(_cam, 45.0f, (float) width / height, 0.1f, 1000.0f);
    h3dResizePipelineBuffers(_hdrPipeRes, width, height);
    h3dResizePipelineBuffers(_forwardPipeRes, width, height);
}


void Application::keyStateHandler() {
    // ----------------
    // Key-press events
    // ----------------
    if (_keys[32] && !_prevKeys[32])  // Space
    {
        if (++_freezeMode == 3) _freezeMode = 0;
    }

    if (_keys[260] && !_prevKeys[260])  // F3
    {
        if (h3dGetNodeParamI(_cam, H3DCamera::PipeResI) == _hdrPipeRes)
            h3dSetNodeParamI(_cam, H3DCamera::PipeResI, _forwardPipeRes);
        else
            h3dSetNodeParamI(_cam, H3DCamera::PipeResI, _hdrPipeRes);
    }

    if (_keys[264] && !_prevKeys[264])  // F7
        _debugViewMode = !_debugViewMode;

    if (_keys[265] && !_prevKeys[265])  // F8
        _wireframeMode = !_wireframeMode;

    if (_keys[263] && !_prevKeys[263])  // F6
    {
        _statMode += 1;
        if (_statMode > H3DUTMaxStatMode) _statMode = 0;
    }

    if (_keys['E'] && !_prevKeys['E'])
    {
        _statMode += 1;
        if (_statMode > H3DUTMaxStatMode) _statMode = 0;
    }
}


void Application::mouseMoveEvent( float dX, float dY ) {
    if (_freezeMode == 2) return;

    // Look left/right
    _ry -= dX / 100 * 30;

    // Loop up/down but only in a limited range
    _rx += dY / 100 * 30;
    if (_rx > 90) _rx = 90;
    if (_rx < -90) _rx = -90;
}
