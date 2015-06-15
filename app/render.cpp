#include <memory>

#include "render.h"

#include "..\common\camera.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"

namespace fd {
  
bool Render::Initialize() {
  //
  //std::unique_ptr<Shader> overdraw(new Shader());
  //overdraw->AddDynamicMeshCommonSubShaders();
  //if(!overdraw->LoadFromFileDerivedNames("OverDraw")) {
  //  return false;
  //}
  //m_pOverdrawQuaxol = overdraw.release();

  //std::unique_ptr<Shader> sliced(new Shader());
  //sliced->AddDynamicMeshCommonSubShaders();
  //if(!sliced->LoadFromFileDerivedNames("Sliced")) {
  //  return false;
  //}
  //m_pSlicedQuaxol = sliced.release();

  return true;
}
  
void Render::UpdateFrameTime() {
  double totalTime = GetTotalTime();
  _frameTime = totalTime - _lastTotalTime;
  _lastTotalTime = totalTime;
}

void Render::Step() {
  for(auto pCamera : m_cameras) {
    pCamera->Step((float)_frameTime);
  }
}

double Render::GetFrameTime() {
  return _frameTime;
}

double Render::GetTotalTime() {
  return timer_.GetElapsed();
}
  
void Render::AddCamera(Camera* pCamera) {
  m_cameras.push_back(pCamera);
}

void Render::AddScene(Scene* pScene) {
  m_scenes.push_back(pScene);
}

void Render::RenderScene(Camera* pCamera, Scene* pScene) {
  static bool doMultiPass = false;
  if(doMultiPass) {

    // 1st pass of color, depth to fbo
    // 2nd pass of depth - offset <= color blend
    //   draw to overlay fbo
    // additive fullscreen render overlay with capped blending

    // or
    
    // depth pass first to depth fbo
    // then for not clipped
    //   if depth <= render normally
    // for clipped
    //   if depth - offset <= render color blend
    //   but capped 

    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.2f);
  
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    pScene->RenderQuaxols(this, pCamera);
    pScene->RenderGroundPlane(pCamera);

    pScene->RenderDynamicEntities(pCamera);

    glDepthMask(GL_FALSE);

    // 2nd pass for 
    pScene->RenderQuaxols(this, pCamera);

    // restore previous settings
    ToggleAlphaDepthModes(m_alphaDepthMode);
  } else {
    pScene->RenderEverything(this, pCamera);
  }
}

void Render::RenderAllScenesPerCamera() {
  for(const auto pCamera : m_cameras) {
    for(auto pScene : m_scenes) {
      RenderScene(pCamera, pScene);
    }
  }
}
  
void Render::ToggleAlphaDepthModes(EAlphaDepthModes mode) {
  if(mode == EToggleModes) {
    mode = (EAlphaDepthModes)(((int)m_alphaDepthMode + 1) % ((int)ENumAlphaDepthModes));
  }
  m_alphaDepthMode = mode;
  std::string modeName = "dunno";
  switch(m_alphaDepthMode) {
    case AlphaOnDepthOffSrcDest: {
      modeName = "AlphaOnDepthOffSrcDest";
      glEnable(GL_BLEND);
      glAlphaFunc(GL_ALWAYS, 0.0f);
      glDisable(GL_ALPHA_TEST);
        
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDepthFunc(GL_ALWAYS);
      glDisable(GL_DEPTH_TEST);
    } break;
    case AlphaOnDepthOffAdditive: {
      modeName = "AlphaOnDepthOffAdditive";
      glEnable(GL_BLEND);
      glAlphaFunc(GL_ALWAYS, 0.0f);
      glDisable(GL_ALPHA_TEST);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE);

      glDepthFunc(GL_ALWAYS);
      glDisable(GL_DEPTH_TEST);
    } break;
    case AlphaTestDepthOffSrcDest: {
      modeName = "AlphaTestDepthOffSrcDest";
      glEnable(GL_BLEND);
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL, 2.0f / 255.0f);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDepthFunc(GL_ALWAYS);
      glDisable(GL_DEPTH_TEST);
    } break;
    case AlphaTestDepthOnSrcDest: {
      modeName = "AlphaTestDepthOnSrcDest";
      glEnable(GL_BLEND);
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL, 0.2f);
  
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDepthFunc(GL_LESS);
      glEnable(GL_DEPTH_TEST);
    } break;
    case AlphaOffDepthOn: {
      modeName = "AlphaOffDepthOn";
      glDisable(GL_BLEND);
      glAlphaFunc(GL_ALWAYS, 0.0f);
      glDisable(GL_ALPHA_TEST);

      glDepthFunc(GL_LESS);
      glEnable(GL_DEPTH_TEST);
    } break;
  }

  printf("switched to %s\n", modeName.c_str());
}

} // namespace fd