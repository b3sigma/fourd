#pragma once

#include <algorithm>
#include <assert.h>
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
  ComponentBus* m_ownerBus;
  bool _beingDestroyed;

public:
  Component() : m_ownerBus(NULL), _beingDestroyed(false) {}
  inline virtual ~Component();

  void PreBusDelete() {
    // if the bus is deleting, don't do destructor callback stuff
    m_ownerBus = NULL; 
  }
  
  void RegisterBus(ComponentBus* bus) {
    m_ownerBus = bus;
  }

  // Components should hook up data if necessary here
  virtual void OnConnected() {}

  inline void UnregisterAllSignals();

  void SelfDestruct() {
    _beingDestroyed = true;
    UnregisterAllSignals();
  }

  typedef std::vector<std::pair<size_t, DelegateMemento>> TRegisteredSignals;
  TRegisteredSignals _registeredSignals;

  template<typename TSlotClass, typename... TVarArgs>
  void RegisterSignal(const std::string& name, TSlotClass* pClass,
      void (TSlotClass::* pFunc)(TVarArgs... varParameters)) {
    // need to store this delegate and then unconnect later...
    auto signalDelegatePair = m_ownerBus->RegisterSignal(name, pClass, pFunc);
    _registeredSignals.push_back(signalDelegatePair);
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
    RegData(void* data, const std::string& name, 
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
  SignalN<float> _sig_step;

  // Might want to do SignalN_baseclass instead of void*
  // Also I think we are hashing the combination of the
  // string hash and the hashed type. I hate your cpu cycles is why.
  typedef std::unordered_map<size_t, void*> TSignalHash;
  TSignalHash _signals;

public:
  ComponentBus() {
    static std::string step("Step");
    _signals.insert(std::make_pair(HashSignalParamsAndName(
        step, typeid(_sig_step).hash_code()), &_sig_step));
  }

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
  bool RegisterOwnerData(const std::string& name, TData* data, bool permanentStorage) {
    RegData* pReg = new RegData((void*)data, name, typeid(TData).hash_code(),
        permanentStorage);
    auto result = _regHash.insert(std::make_pair(pReg->_name, pReg));
    if(result.second == false) {
      delete pReg; //insert failed! couldn't get emplace to work right
    }
    return result.second;
  }

  // Dammnit, mismatch between pointer types means we need this. Seems like there is a solution
  // that doesn't require two functions, but I'm in a hurry right now.
  template <typename TData>
  bool RegisterOwnerDataPtr(const std::string& name, TData** data, bool permanentStorage) {
    RegData* pReg = new RegData((void*)data, name, typeid(TData).hash_code(),
        permanentStorage);
    auto result = _regHash.insert(std::make_pair(pReg->_name, pReg));
    if(result.second == false) {
      delete pReg; //insert failed! couldn't get emplace to work right
    }
    return result.second;
  }

  template <typename TData>
  void CreateOwnerData(const std::string& name, TData* data) {
  }

  template <typename TData>
  bool GetOwnerData(const std::string& name, bool permanentStorage, TData** data) {
    assert(data != NULL);
    auto regIt = _regHash.find(name);
    if (regIt == _regHash.end())
      return false;

    RegData* pReg = regIt->second;
    assert(pReg->_type_hash == typeid(TData).hash_code());
    if(pReg->_isPermanentStorage || !permanentStorage) {
      // Yeah another void* cast, but the typeid check above provides hope.
      *data = (TData*)pReg->_data;
      return true;
    } else {
      // Don't give out temporary pointers for permanent storage
      return false;
    }
  }

  template <typename TData>
  bool GetOwnerDataPtr(const std::string& name, bool permanentStorage, TData** data) {
    assert(data != NULL);
    auto regIt = _regHash.find(name);
    if (regIt == _regHash.end())
      return false;

    RegData* pReg = regIt->second;
    assert(pReg->_type_hash == typeid(TData).hash_code());
    if(pReg->_isPermanentStorage || !permanentStorage) {
      // Yeah another void* cast, but the typeid check above provides hope.
      *data = *((TData**)pReg->_data);
      return true;
    } else {
      // Don't give out temporary pointers for permanent storage
      return false;
    }
  }

  void AddComponent(Component* baby) {
    _components.push_back(baby);
    baby->RegisterBus(this);
    baby->OnConnected();
  }

  void RemoveComponent(Component* baby) {
    _components.erase(
        std::remove(_components.begin(), _components.end(), baby), 
        _components.end());
  }

  void CleanupDestroyed() {
    for(TComponentList::iterator compIt = _components.begin();
        compIt != _components.end();
        // iterate in loop to avoid delete issues
        ) {
      Component* pComp = *compIt;
      if(pComp->_beingDestroyed) {
        pComp->PreBusDelete();
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

  size_t HashSignalParamsAndName(const std::string& name, size_t typeHash) {
    static std::hash<std::string> stringHasher; // is this silly pre-opt?
    return stringHasher(name) ^ (typeHash << 1);
  }

  template<typename TSignalClass, typename... TSignalParams>
  void SendSignal(const std::string& name, 
      TSignalClass signal, TSignalParams... signalParams) {
    size_t signalHash = HashSignalParamsAndName(
        name, typeid(signal).hash_code());
    auto signalPairIt = _signals.find(signalHash);
    if (signalPairIt != _signals.end()) {
      ((TSignalClass*)signalPairIt->second)->Emit(signalParams...);
    }
  }

  template<typename TSlotClass, typename... TVarArgs>
  std::pair<size_t, DelegateMemento> RegisterSignal(const std::string& name, TSlotClass* pClass,
      void (TSlotClass::* pFunc)(TVarArgs... varParameters)) {

    // So we use the function signature plus the signal name to generate
    // a hash that references the specific signal.
    size_t hashVal = HashSignalParamsAndName(name, 
        typeid(SignalN<TVarArgs...>).hash_code());

    SignalN<TVarArgs...>* pSignaler;
    auto signalerIt = _signals.find(hashVal);
    if (signalerIt == _signals.end()) {
      pSignaler = new SignalN<TVarArgs...>();
      _signals.insert(std::make_pair(hashVal, (void*)pSignaler));
    } else {
      // Here we go from the void* to the real type.
      // A hash collision will definitely cause a crash.
      pSignaler = (SignalN<TVarArgs...>*)signalerIt->second;
    }

    SignalN<TVarArgs...>::_Delegate delegate = pSignaler->Connect(pClass, pFunc); 
    return std::make_pair(hashVal, delegate.GetMemento());
  }

  void UnregisterSignal(size_t signalHash, DelegateMemento delegateMem) {
    auto signalerIt = _signals.find(signalHash);
    if (signalerIt == _signals.end()) {
      assert(false); // should have found it
      return;
    }

    // So here is the ugly...
    // We cast to a different kind of signal to delete
    SignalN<int>* pSignaler = (SignalN<int>*)signalerIt->second;

    // then do the same with the delegate
    DelegateN<void, int> delegate;
    delegate.SetMemento(delegateMem);
    pSignaler->Disconnect(delegate);
  }

};

  inline Component::~Component() {
    UnregisterAllSignals();
    if(m_ownerBus) {
      m_ownerBus->RemoveComponent(this);
    }
  }

  inline void Component::UnregisterAllSignals() {
    if(m_ownerBus) {
      for(auto sigDelegatePair : _registeredSignals) {
        m_ownerBus->UnregisterSignal(
            sigDelegatePair.first, sigDelegatePair.second);        
      }
    }
    _registeredSignals.resize(0);
  }


} //namespace fd
