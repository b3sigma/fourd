//#pragma once
//
//#include "../../common/component.h"
//
//namespace fd {
//
//class Render;
//
//class RenderProviderComponent : public Component {
//public:
//  Render* m_pRender; // not owned
//
//public:
//  RenderProviderComponent(Render* pRender) : m_pRender(pRender) {}
//  
//
//
//  virtual void OnConnected() {
//    RegisterSignal(std::string("/*inputAddQuaxol*/"), this, &QuaxolModifierComponent::OnAddQuaxol);
//  
//  }
//};
//
//}; //namespace fd
