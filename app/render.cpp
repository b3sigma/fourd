#include "render.h"
#include "..\common\camera.h"
#include "texture.h"

namespace fd {

  void Render::ToggleAlphaDepthModes(EAlphaDepthModes mode) {
    if(mode == EToggleModes) {
      mode = (EAlphaDepthModes)(((int)m_alphaDepthMode + 1) % ((int)ENumAlphaDepthModes));
    }
    m_alphaDepthMode = mode;
    switch(m_alphaDepthMode) {
      case AlphaOnDepthOffSrcDest: {
        glEnable(GL_BLEND);
        glAlphaFunc(GL_ALWAYS, 0.0f);
        glDisable(GL_ALPHA_TEST);
        
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthFunc(GL_ALWAYS);
        glDisable(GL_DEPTH_TEST);
      } break;
      case AlphaOnDepthOffAdditive: {
        glEnable(GL_BLEND);
        glAlphaFunc(GL_ALWAYS, 0.0f);
        glDisable(GL_ALPHA_TEST);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        glDepthFunc(GL_ALWAYS);
        glDisable(GL_DEPTH_TEST);
      } break;
      case AlphaTestDepthOnSrcDest: {
        glEnable(GL_BLEND);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GEQUAL, 154.0f / 255.0f);
  
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthFunc(GL_LESS); // GL_GREATER); //GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
      } break;
      case AlphaOffDepthOn: {
        glDisable(GL_BLEND);
        glAlphaFunc(GL_ALWAYS, 0.0f);
        glDisable(GL_ALPHA_TEST);

        glDepthFunc(GL_LESS); // GL_GREATER); //GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
      } break;
    }
  }

  } // namespace fd