#include <memory>

#include "render.h"

#include "../common/camera.h"
#include "../common/misc_defs.h"
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
    , m_overdrawColor(NULL)
    //, m_overdrawDepth(NULL)
    , m_renderColor(NULL)
    , m_renderDepth(NULL)
    , m_bufferWidth(0)
    , m_bufferHeight(0)
    , m_viewWidth(0)
    , m_viewHeight(0)
    , m_usingVR(false)
{
  timer_.Start();
  WasGLErrorPlusPrint();

}

Render::~Render() {
  delete m_overdrawColor;
  //delete m_overdrawDepth;
  delete m_renderColor;
  delete m_renderDepth;
  delete m_pOverdrawQuaxol;
  delete m_pSlicedQuaxol;
  delete m_pComposeRenderTargets;
}

bool Render::Initialize(int width, int height) {
  m_bufferWidth = width;
  m_bufferHeight = height;
  UpdateViewHeightFromBuffer();

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
      "Compose", "data/uivCompose.glsl", "data/uifCompose.glsl")) {
    return false;
  }
  m_pComposeRenderTargets = compose.release();

  if(!ResizeRenderTargets(width, height))
    return false;

  if(!InitializeComposeVerts())
    return false;

  return true;
}

bool Render::ResizeRenderTargets(int width, int height) {
  if(m_overdrawColor) {
    if(m_overdrawColor->m_width == width && m_overdrawColor->m_height == height) {
      return true; // success but nothing is true I guess
    }
  }

  m_bufferWidth = width;
  m_bufferHeight = height;
  UpdateViewHeightFromBuffer();

  DEL_NULL(m_overdrawColor);
  //DEL_NULL(m_overdrawDepth);
  DEL_NULL(m_renderColor);
  DEL_NULL(m_renderDepth);


  if(!m_multiPass || width == 0 || height == 0)
    return true; // don't need any of these unless it's multipass

  std::unique_ptr<Texture> colorOverdraw(new Texture());
  if(!colorOverdraw->CreateColorTarget(width, height))
    return false;

  //std::unique_ptr<Texture> depthOverdraw(new Texture());
  //if(!depthOverdraw->CreateDepthTarget(width, height))
  //  return false;

  std::unique_ptr<Texture> renderColor(new Texture());
  if(!renderColor->CreateColorTarget(width, height))
    return false;

  std::unique_ptr<Texture> renderDepth(new Texture());
  if(!renderDepth->CreateDepthTarget(width, height))
    return false;

  m_overdrawColor = colorOverdraw.release();
  //m_overdrawDepth = depthOverdraw.release();
  m_renderColor = renderColor.release();
  m_renderDepth = renderDepth.release();
  return true;
}

void Render::UpdateViewHeightFromBuffer() {
  // uh, this turned out to be wrong, needs investigation
  //if(m_usingVR) {
  //  m_viewWidth = m_bufferWidth / 2;
  //  m_viewHeight = m_bufferHeight / 2;
  //} else {
    m_viewWidth = m_bufferWidth;
    m_viewHeight = m_bufferHeight;
  //}
}

void Render::SetIsUsingVR(bool usingVR) {
  m_usingVR = usingVR;
  UpdateViewHeightFromBuffer();
}

void Render::UpdateFrameTime() {
  double totalTime = GetTotalTime();
  _frameTime = totalTime - _lastTotalTime;
  _lastTotalTime = totalTime;

  if(_frameTime > 0.1f) {
    _frameTime = 0.1f; // below 10fps, leak real time
  }
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

Camera* Render::GetFirstCamera() {
  if(m_cameras.empty())
    return NULL;

  return m_cameras.front();
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

    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    WasGLErrorPlusPrint();

    m_pSlicedQuaxol->StartUsing();
    // TODO: calc this from the block size and the w near and far
    // currently tuned for -40 near, 40 far, 10 blocksize
    static Vec4f sliceRange(0.456f, 0.556f, 0.0f, 0.0f);
    GLuint hSliceShaderRange = m_pSlicedQuaxol->getUniform("sliceRange");
    if(hSliceShaderRange != -1) {
      glUniform4fv(hSliceShaderRange, 1, sliceRange.raw());
      WasGLErrorPlusPrint();
    }
    m_pSlicedQuaxol->StopUsing();

    //float savedWnear = pCamera->_wNear;
    //float savedWfar = pCamera->_wFar;
    //float savedWratio = pCamera->_wScreenSizeRatio;
    //static float sliceAmount = 0.334f;
    //float wRange = pCamera->_wFar - pCamera->_wNear;
    //float wPreNear = (1.0f - sliceAmount) * 0.5f * wRange;
    //pCamera->SetWProjection(savedWnear + wPreNear, savedWfar - wPreNear, 1.0f /*ratio*/);

    pScene->RenderQuaxols(pCamera, m_pSlicedQuaxol);
    pScene->RenderGroundPlane(pCamera);

    //pCamera->SetWProjection(savedWnear, savedWfar, savedWratio);

    // 2nd pass of depth - offset <= color blend to (overdrawfbo)
    glBindFramebuffer(GL_FRAMEBUFFER, m_overdrawColor->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_overdrawColor->m_texture_id, 0);
    //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
    //    GL_TEXTURE_2D, m_overdrawDepth->m_texture_id, 0); // waste?
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // good opportunity to do order independent alpha
    // but first the stupid way
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    WasGLErrorPlusPrint();

    m_pOverdrawQuaxol->StartUsing();
    GLuint hOverdrawShaderRange = m_pOverdrawQuaxol->getUniform("sliceRange");
    if(hOverdrawShaderRange != -1) {
      glUniform4fv(hOverdrawShaderRange, 1, sliceRange.raw());
    }
    m_pOverdrawQuaxol->StopUsing();

    GLint hDepthTex = m_pOverdrawQuaxol->getUniform("texDepth");
    if(hDepthTex != -1) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, pRenderDepth->GetTextureID());
      WasGLErrorPlusPrint();
      //glUniform1i(hDepthTex, 1);
      //WasGLErrorPlusPrint();
    }
    pScene->RenderQuaxols(pCamera, m_pOverdrawQuaxol);

    //// 3rd additive fullscreen render overlay
    ////   with capped blending to ([eyefbo,bb])
    //RenderCompose(pCamera, pRenderColor, m_overdrawColor);

    // restore depth mask
    glDepthMask(GL_TRUE);

  } else {
    pScene->RenderEverything(pCamera);
  }
}

void Render::RenderAllScenesPerCamera(
    Texture* pRenderColor, Texture* pRenderDepth) {

  WasGLErrorPlusPrint();

  Texture* pColorDestination = pRenderColor;
  if(m_multiPass) {
    GLuint clearFlags = 0;
    if(pRenderColor) {
      // this happens when we switch to vr, as the vr rendertarget is different
      ResizeRenderTargets(pRenderColor->m_width, pRenderColor->m_height);
      WasGLErrorPlusPrint();
    } else {
      pRenderColor = m_renderColor;
      glBindFramebuffer(GL_FRAMEBUFFER, pRenderColor->m_framebuffer_id);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
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
      //glClearColor(1.0f, 0.5f, 0.0f, 1.0f);
      glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
      glClear(clearFlags);
      WasGLErrorPlusPrint();
    }
  }

  for(const auto pCamera : m_cameras) {
    for(auto pScene : m_scenes) {
      RenderScene(pCamera, pScene, pRenderColor, pRenderDepth);
    }
  }

  if(m_multiPass) {
    RenderCompose(pColorDestination, pRenderColor, m_overdrawColor);

    // restore previous settings
    ToggleAlphaDepthModes(m_alphaDepthMode);

    for(const auto pCamera : m_cameras) {
      for(auto pScene : m_scenes) {
        pScene->RenderDynamicEntities(pCamera);
      }
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
      //glDepthMask(GL_FALSE);
    } break;
    case AlphaOnDepthOffAdditive: {
      modeName = "AlphaOnDepthOffAdditive";
      glEnable(GL_BLEND);
      glAlphaFunc(GL_ALWAYS, 0.0f);
      glDisable(GL_ALPHA_TEST);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE);

      glDepthFunc(GL_ALWAYS);
      glDisable(GL_DEPTH_TEST);
      //glDepthMask(GL_FALSE);
    } break;
    case AlphaTestDepthOffSrcDest: {
      modeName = "AlphaTestDepthOffSrcDest";
      glEnable(GL_BLEND);
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL, 2.0f / 255.0f);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDepthFunc(GL_ALWAYS);
      glDisable(GL_DEPTH_TEST);
      //glDepthMask(GL_FALSE);
    } break;
    case AlphaTestDepthOnSrcDest: {
      modeName = "AlphaTestDepthOnSrcDest";
      glEnable(GL_BLEND);
      glEnable(GL_ALPHA_TEST);
      glAlphaFunc(GL_GEQUAL, 0.2f);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glDepthFunc(GL_LESS);
      glEnable(GL_DEPTH_TEST);
      //glDepthMask(GL_TRUE);
    } break;
    case AlphaOffDepthOn: {
      modeName = "AlphaOffDepthOn";
      glDisable(GL_BLEND);
      glAlphaFunc(GL_ALWAYS, 0.0f);
      glDisable(GL_ALPHA_TEST);

      glDepthFunc(GL_LESS);
      glEnable(GL_DEPTH_TEST);
      //glDepthMask(GL_TRUE);
    } break;
  }

  //printf("switched to %s\n", modeName.c_str());
}

bool Render::InitializeComposeVerts() {
  m_composeVerts[0] = {-1.0f, -1.0f}; //, 0.0f, 0.0f};
  m_composeVerts[1] = {+1.0f, -1.0f}; //, 1.0f, 0.0f};
  m_composeVerts[2] = {-1.0f, +1.0f}; //, 0.0f, 1.0f};

  m_composeVerts[3] = {-1.0f, +1.0f}; //, 0.0f, 1.0f};
  m_composeVerts[4] = {+1.0f, -1.0f}; //, 1.0f, 0.0f};
  m_composeVerts[5] = {+1.0f, +1.0f}; //, 1.0f, 1.0f};

  return true;
}

void Render::RenderCompose(Texture* pDestination,
    Texture* pRenderColor, Texture* pOverdrawSource) {
  if(pDestination == NULL) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, pDestination->m_framebuffer_id);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, pDestination->m_texture_id, 0);
  }

  m_pComposeRenderTargets->StartUsing();

  //GLint texCoordIndex = glGetAttribLocation(
  //    m_pComposeRenderTargets->getProgramId(), "vertCoord");
  GLint hSolid = m_pComposeRenderTargets->getUniform("texSolid");
  if(hSolid != -1) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pRenderColor->GetTextureID());
    glUniform1i(hSolid, 0);
  }
  GLint hOverdraw = m_pComposeRenderTargets->getUniform("texOverdraw");
  if(hOverdraw != -1) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, pOverdrawSource->GetTextureID());
    glUniform1i(hOverdraw, 1);
  }

  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  glBegin(GL_TRIANGLES);
  for(int t = 0; t < 2; t++) {
    int v = t * 3;
    //glVertexAttrib2f(texCoordIndex, m_composeVerts[v+0].u, m_composeVerts[v+0].v);
    glVertex2f(                     m_composeVerts[v+0].x, m_composeVerts[v+0].y);
    //glVertexAttrib2f(texCoordIndex, m_composeVerts[v+1].u, m_composeVerts[v+1].v);
    glVertex2f(                     m_composeVerts[v+1].x, m_composeVerts[v+1].y);
    //glVertexAttrib2f(texCoordIndex, m_composeVerts[v+2].u, m_composeVerts[v+2].v);
    glVertex2f(                     m_composeVerts[v+2].x, m_composeVerts[v+2].y);
  }
  glEnd();

  m_pComposeRenderTargets->StopUsing();
}

void Render::ToggleMultipassMode(bool multiPass, int width, int height) {
  m_multiPass = multiPass;

  if(m_multiPass) {
    ResizeRenderTargets(width, height);
  } else {
    ResizeRenderTargets(0, 0);
  }

}

} // namespace fd
