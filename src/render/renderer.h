#ifndef INCLUDED_RENDER_RENDERER_H
#define INCLUDED_RENDER_RENDERER_H
#include "platform/i_platform.h"
#include "render/i_render.h"

namespace engine {
class RendererSystem : public System
{
    ~RendererSystem();

    Projection mWorldProjector;
    Projection mUiProjector;
    Camera mCamera;
    Ui& mUi;
    SceneRenderer mSceneRenderer;
    ActorRenderer mActorRenderer;
    UiRenderer mUiRenderer;
    DecalEngine& mDecalEngine;
    ShaderManager& mShaderManager;

    AutoReg mMouseMoveId;
    AutoReg mMousePressId;
    AutoReg mMouseReleaseId;

    bool BeginRender();
    bool EndRender();
    void SetupRenderer( const Projection& Proj );
    // ez logikailag nem tuti, hogy teljesen korrekt, de a renderer az, aki ismeri a projectionoket
    // ki lehetne belole kulon osztalyba rakni esetleg
    // a screen mouse eventeket mindenesetre ezen a reszen lehet csak kezelni, a rendererben (hisz csak o tudja, h egyaltalan mi az a screen)
    void OnMouseMoveEvent( const ScreenMouseMoveEvent& Event );
    void OnMousePressEvent( const ScreenMousePressEvent& Event );
    void OnMouseReleaseEvent( const ScreenMouseReleaseEvent& Event );

public:
    RendererSystem();
    DEFINE_SYSTEM_BASE(RendererSystem)
    virtual void Init();
    virtual void Update( double DeltaTime );
};
} // namespace engine

#endif//INCLUDED_RENDER_RENDERER_H
