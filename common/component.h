#pragma once

#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "thirdparty/signals/Signal.h"

namespace fd {

// The component bus goes in the owner class.
// It has a list of components, linked together by a signal/slot thing.
// The owner should register relevant data that can be accessed by
//  components through the bus, including functions.
// Timed messages can be delivered through the bus to components and from them.

class ComponentBus;
class Component {
protected:
  friend ComponentBus;
  ComponentBus* _bus;
  bool _beingDestroyed;
public:
  Component() : _bus(NULL), _beingDestroyed(false) {}
  virtual ~Component() {}

  void PreBusDelete() {}
  
  void RegisterBus(ComponentBus* bus) {
    _bus = bus;
  }

  // Components should hook up data if necessary here
  virtual void OnConnected() {}

  void SelfDestruct() {
    _beingDestroyed = true;
  }
};
typedef std::vector<Component*> TComponentList;

class ComponentBus {
protected:
  class RegData {
  public:
    RegData() : _data(NULL), 
        _type_hash(0), 
        _isPermanentStorage(false),
        _busOwned(false) {}
    RegData(void* data, const char* name, 
        size_t type_hash,
        bool permanentStorage) 
        : _data(data), _name(name),
        _type_hash(type_hash),
        _isPermanentStorage(permanentStorage),
        _busOwned(false) {}

    void* _data; // type safety is only asserted at runtime
    std::string _name;
    size_t _type_hash; // type checking help, maybe debug only?
    bool _isPermanentStorage; // whether components can retain pointers to this
    bool _busOwned; // used for data that was created by the bus
  };
  typedef std::unordered_map<std::string, RegData*> TRegHash;
  TRegHash _regHash;

  TComponentList _components;
  Signal1<float> _sig_step;

public:
  ComponentBus() {}
  ~ComponentBus() {
    for(auto& pComponent : _components) {
      pComponent->PreBusDelete();
    }
    for(auto& pComponent : _components) {
      delete pComponent;
    }

    for(auto& regIt : _regHash) {
      auto pReg = regIt.second;
      if(pReg->_busOwned) {
        delete pReg;
      }
    }
  }

  template <typename TData>
  bool RegisterOwnerData(const char* name, TData* data, bool permanentStorage) {
    RegData* pReg = new RegData((void*)data, name, typeid(TData).hash_code(),
        permanentStorage);
    auto result = _regHash.insert(std::make_pair(pReg->_name, pReg));
    if(result.second == false) {
      delete pReg; //insert failed! couldn't get emplace to work right
    }
    return result.second;
  }

  template <typename TData>
  void CreateOwnerData(const char* name, TData* data) {
  }

  template <typename TData>
  bool GetOwnerData(const char* name, bool permanentStorage, TData** data) {

    return false;
  }

  void AddComponent(Component* baby) {
    _components.push_back(baby);
    baby->RegisterBus(this);
    baby->OnConnected();
  }

  void CleanupDestroyed() {
    for(TComponentList::iterator compIt = _components.begin();
        compIt != _components.end();
        // iterate in loop to avoid delete issues
        ) {
      Component* pComp = *compIt;
      if(pComp->_beingDestroyed) {
        delete pComp;
        compIt = _components.erase(compIt);
      } else {
        ++compIt;
      }
    }
  }

  // Associated with Component::SelfDestruct
  void PreComponentDelete() {}

  // Component bus needs to be stepped in order to update messages
  void Step(float delta) {
    CleanupDestroyed(); // avoid stuff that's about to die stepping
    _sig_step.Emit(delta);
    CleanupDestroyed(); // cleanup stuff that died
  }

  // The sub class of component has to register each signal
  // with the name of the signal plus a function pointer.
  // Then the bus needs to be able to message these signals
  // with parameters.
  // The signal should be stored on the bus. However, the component needs
  // to know all the signals it has registered, so it can un-register on
  // destruction.

  //template<TSignalType>
  //void RegisterSignal(const char* name, void* pClass, 
  //    _bus->RegisterSignal("Step", this, &::SuicideComponent::OnStepSignal);

};

//template<typename... TVarArgs>
//class Signal {
//
//};
//
//template<typename TSlotClass, typename... TVarArgs>
//void RegisterSignal(const char* name, void* pClass,
//    void (TSlotClass::*func)(TVarArgs... varParameters)) {
//  
//}

} //namespace fd
