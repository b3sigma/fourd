#include "tweak.h"
#include "tweak_registrar.h"

namespace fd {

TweakRegistrar::HashTweaks* TweakRegistrar::_nameToTweak = NULL;

void TweakRegistrar::RegisterTweak(const char* name, TweakVariable* tweak) {
  LeakSomeSingleton();
  if(_nameToTweak) {
    _nameToTweak->insert(std::make_pair(std::string(name), tweak));
  }
}

void TweakRegistrar::UnregisterTweak(const char* name, TweakVariable* tweak) {
  if(_nameToTweak) {
    _nameToTweak->erase(std::string(name));
  }
}

void TweakRegistrar::LeakSomeSingleton() {
  if(!_nameToTweak) {
    _nameToTweak = new TweakRegistrar::HashTweaks();
  }
}

} // namespace fd 