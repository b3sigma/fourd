#include <memory>
#include <vector>
#include "pyvis.h" // has FD_USE_PYTHON_HOOK 

#ifdef FD_USE_PYTHON_HOOK

#include <python2.7/Python.h>
#include "../common/timer.h"


namespace fd {

class PyVis {
public:
    PyObject* potentialDict; 
    PyObject* potentialMod;
    PyObject* potentialStep;

    PyVis() : potentialDict(NULL), potentialMod(NULL), potentialStep(NULL) {}
    ~PyVis() {
        Py_CLEAR(potentialStep);
        Py_CLEAR(potentialDict);
        Py_CLEAR(potentialMod);
    }
};

struct ScopePyClear {
    PyObject*& pyObj;
    ScopePyClear(PyObject*& obj) : pyObj(obj) {
        printf("made a ScopePyClear!\n");
    } 
    ~ScopePyClear() { Py_CLEAR(pyObj); 
        printf("boomed a ScopePyClear!\n");
    }
};

// // from http://the-witness.net/news/2012/11/scopeexit-in-c11/
// template <typename F>
// struct ScopeExit {
//     ScopeExit(F f) : f(f) {}
//     ~ScopeExit() { f(); }
//     F f;
// };

// template <typename F>
// ScopeExit<F> MakeScopeExit(F f) {
//     return ScopeExit<F>(f);
// };

// #define SCOPE_EXIT(code) \
//     auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})


static PyVis* g_PyVis = NULL;
bool PyVisInterface::InitPython() {
    if(0 != Py_IsInitialized() || g_PyVis != NULL)
        return true;

    std::unique_ptr<PyVis> pyVis(new PyVis()); 

    Py_Initialize();
    // PyObject* mainModule = PyImport_AddModule("__main__");
    // //ScopePyClear mmClear(mainModule);
    // if(!mainModule || !PyModule_Check(mainModule)) { 
    //     printf("Error loading python main module:\n");
    //     Py_CLEAR(mainModule);
    //     PyErr_Print();
    //     return false;
    // }
    
    // PyRun_SimpleString("print 'Then...'");
    
    // PyObject* mainDict = PyModule_GetDict(mainModule);
    // pyVis->mainDict = mainDict;
    // if(!mainDict || !PyDict_Check(mainDict)) {
    //     printf("Error loading python main dictionary:\n");
    //     PyErr_Print();
    //     return false; 
    // }
    // Py_CLEAR(mainModule);
    
    PyObject* systemModule = PyImport_ImportModule("sys");
    if(!systemModule || !PyModule_Check(systemModule)) {
        printf("Error loading python module sys:\n");
        PyErr_Print();
        Py_CLEAR(systemModule);
        return false;
    }


    PyObject* sysPath = PyObject_GetAttrString(systemModule, "path");
    if(!sysPath || !PyList_Check(sysPath)) {
        printf("Error loading python sys path:\n");
        PyErr_Print();
        Py_CLEAR(sysPath);
        return false;
    }
    PyList_Append(sysPath, PyString_FromString("pyvis"));
    PyList_Append(sysPath, PyString_FromString("pyvis/PIMCPy"));

    PyObject* potentialMod = PyImport_ImportModuleEx("TestSingleSlicePotential",
            /*globals*/ NULL, /*locals*/ NULL, /*fromlist*/ NULL);
    pyVis->potentialMod = potentialMod;
    if(!potentialMod || !PyModule_Check(potentialMod)) {
        printf("Error loading TestSingleSlicePotential Module\n");
        if(PyErr_Occurred()) { PyErr_Print(); }
        Py_CLEAR(potentialMod);
        return false;
    }
    
    PyObject* potentialDict = PyModule_GetDict(potentialMod);
    pyVis->potentialDict = potentialDict;
    if(!potentialDict || !PyDict_Check(potentialDict)) {
        printf("Error loading python main dictionary:\n");
        PyErr_Print();
        return false; 
    }

    PyRun_String("ScriptCreate()", Py_eval_input, potentialDict, potentialDict);
    PyRun_SimpleString("print 'afterbutThen...'");

    PyObject* scriptStep = PyDict_GetItemString(potentialDict, "ScriptStep");
    pyVis->potentialStep = scriptStep;
    if(!scriptStep || !PyCallable_Check(scriptStep)) {
        printf("Error loading python potential step function linkage:\n");
        PyErr_Print();
        return false; 
    }

    if(0 == Py_IsInitialized()) {
        return false;
    }

    g_PyVis = pyVis.release();
    return true;
}

void PyVisInterface::ShutdownPython() {
    delete g_PyVis;
    g_PyVis = NULL;

    Py_Finalize();
}


bool PyVisInterface::PathIntegralSingleStep(NumberList& output) {
    Timer slowness("python time");
    // SCOPE_EXIT(printf("does this scope_exit thing even work?\n"));
    if(!g_PyVis || !g_PyVis->potentialDict || !g_PyVis->potentialStep)
        return false;

    printf("precallobject\n");
    PyObject* result = PyObject_CallObject(g_PyVis->potentialStep, /* args */ NULL);
    ScopePyClear clearResult(result);

    if(result && PyList_Check(result)) {
        int numResults = (int)PyList_Size(result);
        output.reserve(output.size() + numResults);
        for(int i = 0; i < numResults; i++) {
            PyObject* item = PyList_GetItem(result, i);
            ScopePyClear clearItem(item);
            if(!item || PyFloat_Check(item)) continue;

            output.push_back(PyFloat_AsDouble(item));
        }
    }

    return true;
}

bool PyVisInterface::RunTests() {
    return true; // so... the init/shutdown thing only works once?

    assert(InitPython() == true);
    ShutdownPython();
    assert(InitPython() == true);

    PyRun_SimpleString("import numpy");

    PyRun_SimpleString("print 'Python is wokring, numpy.sqrt gives %f' % (numpy.sqrt(13))");
    // actually testing something would be pretty cool

    ShutdownPython();

    return true;
}

} // namespace fd

#endif //def FD_USE_PYTHON_HOOK
