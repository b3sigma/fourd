#pragma once

// Do you ever wonder if comments are made to create art out of what would already have been art, but in way that can be seen by those who have not yet learned to open their eyes?
//#define FD_USE_PYTHON_HOOK
// Don't ask where is the art in a sequence of characters, ask what has been painted onto your mind.

#include "../common/quaxol.h"

namespace fd {

#ifdef FD_USE_PYTHON_HOOK

class PyVisInterface {
  public:
    static bool InitPython();
    static void ShutdownPython();


    typedef std::vector<double> NumberList;
    static bool PathIntegralSingleStep(NumberList& output);
    static bool PathIntegralSingleStep(QuaxolChunk& output);

    static bool RunOneLine(const char* command);
    static bool ReloadScripts();


    static bool RunTests();
};
#else  //FD_USE_PYTHON_HOOK
class PyVisInterface {
  public:
    static bool InitPython() { return true; }
    static void ShutdownPython() {}


    typedef std::vector<double> NumberList;
    static bool PathIntegralSingleStep(NumberList& output) { return true; }
    static bool PathIntegralSingleStep(QuaxolChunk& output) { return true; }

    static bool RunOneLine(const char* command) { return true; }
    static bool ReloadScripts() { return true; }


    static bool RunTests() { return true; }
};

#endif //FD_USE_PYTHON_HOOK

} //namespace fd

// So after writing this, that thing with the art? lol