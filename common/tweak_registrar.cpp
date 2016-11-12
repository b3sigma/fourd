#include "tweak.h"
#include "tweak_registrar.h"

namespace fd {

TweakRegistrar::HashTweaks TweakRegistrar::_nameToTweak;

void TweakRegistrar::RegisterTweak(const char* name, TweakVariable* tweak) {
  _nameToTweak.insert(std::make_pair(std::string(name), tweak));
}

void TweakRegistrar::UnregisterTweak(const char* name, TweakVariable* tweak) {
  _nameToTweak.erase(std::string(name));
}

} // namespace fd 