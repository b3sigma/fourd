#pragma once

#include "../../common/component.h"
#include "../../common/fourmath.h"

namespace fd {

class Scene;

class QuaxolModifierComponent : public Component {
public:
  int m_currentQuaxolType;

  Mat4f* m_pOwnerOrientation; // note as this is 4d we need full 4d matrix for orientation
  Vec4f* m_pOwnerPosition; // also need full vec4 for position.
  Scene* m_pOwnerScene;

public:
  QuaxolModifierComponent() : m_currentQuaxolType(0) {}
  
  virtual void OnConnected();

  // because I suck, all input callbacks take a frametime
  void OnAddQuaxol(float frameTime);
  void OnRemoveQuaxol(float frameTime);
  void OnNextCurrentItem(float frameTime);
  void OnPrevCurrentItem(float frameTime);
};

}; //namespace fd
