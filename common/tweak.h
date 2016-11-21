#pragma once

#include <string>

namespace fd {

class TweakVariable {
public:
  enum TweakType {
    TweakFloat,
    TweakDouble,
    TweakInt,
    TweakBool
  };
  union {
    float   _floatVal;
    double  _doubleVal;
    int     _intVal;
    bool    _boolVal;
    double  _largestVal; //if you add a larger datatype than a double, change this datatype to the new one
  };
  std::string _name;
  TweakType _type;

  TweakVariable(const char* name, double startVal);
  TweakVariable(const char* name, float startVal);
  TweakVariable(const char* name, int startVal);
  TweakVariable(const char* name, bool startVal);

  TweakVariable(const char* name);
  ~TweakVariable();

  TweakType GetType() const { return _type; }

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