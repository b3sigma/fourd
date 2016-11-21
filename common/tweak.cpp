#include "tweak.h"
#include "tweak_registrar.h"

namespace fd {

TweakVariable::TweakVariable(const char* name, double startVal)
    : TweakVariable(name) { 
  _doubleVal = startVal; 
  _type = TweakDouble;
}

TweakVariable::TweakVariable(const char* name, float startVal)
    : TweakVariable(name) { 
  _floatVal = startVal; 
  _type = TweakFloat;
}

TweakVariable::TweakVariable(const char* name, int startVal)
    : TweakVariable(name) { 
  _intVal = startVal;
  _type = TweakInt;
}

TweakVariable::TweakVariable(const char* name, bool startVal)
    : TweakVariable(name) { 
  _boolVal = startVal;
  _type = TweakBool;
}

TweakVariable::TweakVariable(const char* name) 
    : _name(name)
    , _largestVal(0) {
  TweakRegistrar::RegisterTweak(name, this);
}

TweakVariable::~TweakVariable() {
  TweakRegistrar::UnregisterTweak(_name.c_str(), this);
}

double* TweakVariable::AsDoublePtr() { return &_doubleVal; }
int* TweakVariable::AsIntPtr() { return &_intVal; }
float* TweakVariable::AsFloatPtr() { return &_floatVal; }
bool* TweakVariable::AsBoolPtr() { return &_boolVal; }

double& TweakVariable::AsDouble() { return _doubleVal; }
int& TweakVariable::AsInt() { return _intVal; }
float& TweakVariable::AsFloat() { return _floatVal; }
bool& TweakVariable::AsBool() { return _boolVal; }

} //namespace fd