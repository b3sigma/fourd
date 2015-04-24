
#include <Windows.h>

#define STB_DEFINE
#pragma warning(push)
#pragma warning(disable: 4996)  // disable warning about _s functions
#include "../stb/stb.h"
#pragma warning(pop)

#include "shader.h"

namespace fd {



TShaderHash* Shader::s_pShaderhash = NULL;


#define SHADER_HASH_EMPTY ((const char*)(1))
#define SHADER_HASH_DEL ((const char*)(2))

// Ugh, this is stupid. Would rather std::string but stb_hash is malloc based.
const char* shader_hash_str_copy(const char* str) {
  if (str == NULL) return NULL;
  if (str == SHADER_HASH_EMPTY) return SHADER_HASH_EMPTY;
  if (str == SHADER_HASH_DEL) return SHADER_HASH_DEL;

  unsigned int size = strlen(str) + 1;
  char* newStr = new char[size];
  strcpy_s(newStr, size, str);
  return newStr;
}

void shader_hash_str_del(const char* str) {
  delete [] str;
}

bool shader_hash_str_cmp(const char* left, const char* right) {
  if (left == right) { return true; }
  if (left == NULL || left == SHADER_HASH_EMPTY || left == SHADER_HASH_DEL ||
      right == NULL || right == SHADER_HASH_EMPTY || right == SHADER_HASH_DEL) {
    return false;
    //return (left < right) ? -1 : 1;
  }

  return _stricmp(left, right) == 0;
}

stb_define_hash_base(STB_noprefix, TShaderHash, STB_nofields,
  shader_hash_, shader_hash_, 0.85f, const char*, 
  SHADER_HASH_EMPTY, SHADER_HASH_DEL, 
  shader_hash_str_copy, shader_hash_str_del, STB_nosafe,
  shader_hash_str_cmp, shader_hash_str_cmp,
  return stb_hash(k); , Shader*, STB_nullvalue, NULL);

int Shader::s_shader_refs = 0;

bool Shader::TestShaderHash() {
  TShaderHash* pTestHash = shader_hash_create();

  char hashTest1[] = "shady0";
  char hashTest2[] = "shady1";
  assert(stb_hash(&hashTest1[0]) != stb_hash(&hashTest2[0]));
  
  int numEntries = 100;
  for (int test = 0; test < numEntries; ++test) {
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", test);
    Shader* pShader = new Shader(std::string(name));
    
    assert(0 == shader_hash_update(pTestHash, name, pShader));
    assert(1 == shader_hash_add(pTestHash, name, pShader));
    assert(0 == shader_hash_add(pTestHash, name, pShader));
    //assert(1 == shader_hash_set(pTestHash, name, pShader));
    //assert(1 == shader_hash_update(pTestHash, name, pShader));

    assert(shader_hash_get(pTestHash, name) == pShader);
    assert(shader_hash_get(pTestHash, std::string(name).c_str()) == pShader);
  }

  assert(Shader::s_shader_refs == numEntries);

  for (int test = 0; test < numEntries; ++test) {
    int nameIndex = rand() + numEntries;
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", nameIndex);
    Shader* pShader = new Shader(std::string(name));

    if(1 == shader_hash_add(pTestHash, name, pShader)) {
      assert(shader_hash_get(pTestHash, name) == pShader);
      
      Shader* pToDelete = NULL;
      shader_hash_remove(pTestHash, std::string(name).c_str(), &pToDelete);
      assert(pShader == pToDelete);
    }
    delete pShader;
  }

  assert(Shader::s_shader_refs == numEntries);

  for (int test = 0; test < numEntries; ++test) {
    char* name = new char[256];
    sprintf_s(&(name[0]), sizeof(char[256]), "shady%d", test);

    Shader* pShader = shader_hash_get(pTestHash, std::string(name).c_str());

    Shader* pToDelete = NULL;
    shader_hash_remove(pTestHash, std::string(name).c_str(), &pToDelete);
    delete pToDelete;
    delete [] name;
  }

  assert(Shader::s_shader_refs == 0);
  shader_hash_destroy(pTestHash);
  return true;
}



}

