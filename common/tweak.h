#pragma once

#include <string>

namespace fd {

class TweakVariable {
public:
  union {
    float   _floatVal;
    double  _doubleVal;
    int     _intVal;
    bool    _boolVal;
  };
  std::string _name;

  TweakVariable(const char* name, double startVal);
  TweakVariable(const char* name, float startVal);
  TweakVariable(const char* name, int startVal);
  TweakVariable(const char* name, bool startVal);

  TweakVariable(const char* name);
  ~TweakVariable();

  float* AsFloatPtr();
  double* AsDoublePtr();
  int* AsIntPtr();
  bool* AsBoolPtr();

  float& AsFloat();
  double& AsDouble();
  int& AsInt();
  bool& AsBool();
  
};

} //namespace fd