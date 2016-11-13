#pragma once

#include <unordered_map>

namespace fd {

class TweakVariable;

class TweakRegistrar {
public:
  typedef std::unordered_map<std::string, TweakVariable*> HashTweaks;
  static HashTweaks* _nameToTweak;
public:
  static void RegisterTweak(const char* name, TweakVariable* tweak);
  static void UnregisterTweak(const char* name, TweakVariable* tweak);

  static void LeakSomeSingleton();
};

} // namespace fd