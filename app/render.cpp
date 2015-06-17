#include <memory>

#include "render.h"

#include "..\common\camera.h"
#include "glhelper.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"

namespace fd {

Render::Render() 
    : _frameTime(0.0)
    , _lastTotalTime(0.0)
    , m_multiPass(true)
    , m_pOverdrawQuaxol(NULL)
    , m_pSlicedQuaxol(NULL)
    , m_pComposeRenderTargets(NULL)
    , m_colorOverdraw(NULL)
    , m_renderColor(NULL)
    , m_renderDepth(NULL)
{
}

Render::~Render() {
  delete m_colorOverdraw;
  delete m_renderColor;
  delete m_renderDepth;
  delete m_pOverdrawQuaxol;
  delete m_pSlicedQuaxol;
  delete m_pComposeRenderTargets;
}
  
bool Render::Initialize(int width, int height) {
  
  std::unique_ptr<Shader> overdraw(new Shader());
  overdraw->AddDynamicMeshCommonSubShaders();
  if(!overdraw->LoadFromFileDerivedNames("OverdrawRainbow")) {
    return false;
  }
  m_pOverdrawQuaxol = overdraw.release();

  std::unique_ptr<Shader> sliced(new Shader());
  sliced->AddDynamicMeshCommonSubShaders();
  if(!sliced->LoadFromFileDerivedNames("Sliced")) {
    return false;
  }
  m_pSlicedQuaxol = sliced.release();

  std::unique_ptr<Shader> compose(new Shader());
  if(!compose->LoadFromFile(
      "Compose", "data\\uivCompose.glsl", "data\\uifCompose.glsl")) {
    return false;
  }
  m_pComposeRenderTargets = compose.release();
  
  if(!ResizeRenderTargets(width, height))
    return false;

  if(!InitializeComposeVerts())
    return false;

  m_multiPass = false; //(m_pSlicedQuaxol && m_pOverdrawQuaxol);

  return true;
}

bool Render::ResizeRenderTargets(int width, int height) {
  if(m_colorOverdraw) {
    if(m_colorOverdraw->m_width == width && m_colorOverdraw->m_height == height) {
      return true; // success but nothing is true I guess
    }
  }

  delete m_colorOverdraw;
  delete m_renderColor;
  delete m_renderDepth;
  
  std::unique_ptr<Texture> colorOverdraw(new Texture());
  if(!colorOverdraw->CreateRenderTarget(width, height))
    return false;

  std::unique_ptr<Texture> renderColor(new Texture());
  if(!renderColor->CreateRenderTarget(width, height))
    return false;

  std::unique_ptr<Texture> renderDepth(new Texture());
  if(!renderDepth->CreateDepthTarget(width, height))
    return false;

  m_colorOverdraw = colorOverdraw.release();
  m_renderColor = renderColor.release();
  m_renderDepth = renderDepth.release();
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

void Render::RenderScene(Camera* pCamera, Scene* pScene,
    Texture* pRenderColor, Texture* pRenderDepth) {
  if(m_multiPass && m_pSlicedQuaxol && m_pOverdrawQuaxol
      && pRenderDepth && pRenderColor) {
    // 1st pass of color, depth to ([eyefbo,colorfbo], [eyedepth,depthfbo])
    glDisable(GL_BLEND);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GEQUAL, 0.2f);
    WasGLErrorPlusPrint();
  
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    pScene->RenderQuaxols(pCamera, m_pSlicedQuaxol);
    pScene->RenderGroundPlane(pCamera);

    // 2nd pass of depth - offset <= color blend to (overdrawfbo)
    glBindFramebuffer(GL_FRAMEBUFFER, m_colorOverdraw->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_colorOverdraw->m_texture_id, 0);

    // good opportunity to do order independent alpha
    // but first the stupid way
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  WasGLErrorPlusPrint();

    // right now we are assuming these are all alpha, no depthy
    pScene->RenderDynamicEntities(pCamera);
  WasGLErrorPlusPrint();

    GLint hDepthTex = m_pOverdrawQuaxol->getUniform("texDepth");
    if(hDepthTex != -1) {
      glActiveTexture(GL_TEXTURE1);
  WasGLErrorPlusPrint();
      glBindTexture(GL_TEXTURE_2D, pRenderDepth->GetTextureID());
  WasGLErrorPlusPrint();
  //    glUniform1i(hDepthTex, 1);
  //WasGLErrorPlusPrint();
    }
    pScene->RenderQuaxols(pCamera, m_pOverdrawQuaxol);

    // 3rd additive fullscreen render overlay
    //   with capped blending to ([eyefbo,bb])
    RenderCompose(pCamera, pRenderColor, m_colorOverdraw);

    // restore previous settings
    ToggleAlphaDepthModes(m_alphaDepthMode);
  } else {
    pScene->RenderEverything(pCamera);
  }
}

void Render::RenderAllScenesPerCamera(
    Texture* pRenderColor, Texture* pRenderDepth) {

  WasGLErrorPlusPrint();

  if(m_multiPass) {
    GLuint clearFlags = 0;
    if(pRenderColor) {
      // this happens when we switch to vr, as the vr rendertarget is different
      ResizeRenderTargets(pRenderColor->m_width, pRenderColor->m_height);
      WasGLErrorPlusPrint();
    } else {
      pRenderColor = m_renderColor;
      glBindFramebuffer(GL_FRAMEBUFFER, pRenderColor->m_framebuffer_id);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
          GL_TEXTURE_2D, pRenderColor->m_texture_id, 0);
      clearFlags |= GL_COLOR_BUFFER_BIT;
      WasGLErrorPlusPrint();
    }

    // setup render depth if it wasn't passed in
    if(pRenderDepth == NULL) {
      pRenderDepth = m_renderDepth;
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
          GL_TEXTURE_2D, pRenderDepth->m_texture_id, 0);
      clearFlags |= GL_DEPTH_BUFFER_BIT;
      WasGLErrorPlusPrint();
    }

    if(clearFlags != 0) {
      glViewport(0, 0, pRenderDepth->m_width, pRenderDepth->m_height);
      glClear(clearFlags);
      WasGLErrorPlusPrint();
    }
  }

  for(const auto pCamera : m_cameras) {
    for(auto pScene : m_scenes) {
      RenderScene(pCamera, pScene, pRenderColor, pRenderDepth);
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

struct ComposeVert {
  float x, y;
  float u, v;
};

bool Render::InitializeComposeVerts() {
  m_composeVerts[0] = {0.0f, 0.0f, 0.0f, 0.0f};
  m_composeVerts[1] = {1.0f, 0.0f, 1.0f, 0.0f};
  m_composeVerts[2] = {0.0f, 1.0f, 0.0f, 1.0f};

  m_composeVerts[3] = {0.0f, 1.0f, 0.0f, 1.0f};
  m_composeVerts[4] = {1.0f, 0.0f, 1.0f, 0.0f};
  m_composeVerts[5] = {1.0f, 1.0f, 1.0f, 1.0f};

  return true;
}

void Render::RenderCompose(Camera* pCamera, 
    Texture* pRenderColor, Texture* pOverdrawSource) {
  if(pRenderColor == NULL) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, pRenderColor->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, pRenderColor->m_texture_id, 0);
  }

  m_pComposeRenderTargets->StartUsing();
  m_pComposeRenderTargets->SetCameraParams(pCamera);
  
  GLint hTexCoord = m_pComposeRenderTargets->getAttrib("vertCoord");
  GLint hOverdraw = m_pComposeRenderTargets->getUniform("texOverdraw");
  if(hOverdraw != -1) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pOverdrawSource->GetTextureID());
    //glUniform1i(hOverdraw, 0);
  }

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);

  glBegin(GL_TRIANGLES);
  for(int t = 0; t < 2; t++) {
    int v = t * 3;
    glVertexAttrib2f(hTexCoord, m_composeVerts[v+0].u, m_composeVerts[v+0].v);
    glVertex2f(                 m_composeVerts[v+0].x, m_composeVerts[v+0].y);
    glVertexAttrib2f(hTexCoord, m_composeVerts[v+1].u, m_composeVerts[v+1].v);
    glVertex2f(                 m_composeVerts[v+1].x, m_composeVerts[v+1].y);
    glVertexAttrib2f(hTexCoord, m_composeVerts[v+2].u, m_composeVerts[v+2].v);
    glVertex2f(                 m_composeVerts[v+2].x, m_composeVerts[v+2].y);
  }
  glEnd();

  m_pComposeRenderTargets->StopUsing();
}

} // namespace fd