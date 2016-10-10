#include <memory>
#include <vector>
#include "pyvis.h" // has FD_USE_PYTHON_HOOK 

#ifdef FD_USE_PYTHON_HOOK

#include <map>
#include <python2.7/Python.h>
#include "../common/timer.h"


namespace fd {

struct ScopePyDecRef {
    PyObject* pyObj;
    ScopePyDecRef(PyObject* obj) : pyObj(obj) { } 
    ~ScopePyDecRef() {
        if(pyObj) {
            Py_DECREF(pyObj);
        }
    }
};

class PyVis {
public:
    typedef std::map<std::string, PyObject*> HashToPython;

    HashToPython _modules;
    HashToPython _dictionaries; // borrowed references, don't decref

    PyObject* scriptStep; // borrowed ref, don't decref
    PyObject* consoleGlobalContextDict; // weak ref, don't decref
    PyObject* consoleLocalContextDict; // weak ref, don't decref

    PyVis() : scriptStep(NULL)
        , consoleGlobalContextDict(NULL)
        , consoleLocalContextDict(NULL) {}
    ~PyVis() {
        _dictionaries.clear(); // borrowed refs
        DecRefPythonObjects(_modules);
        _modules.clear();
    }

    PyObject* GetModule(std::string moduleName) {
        auto findIt = _modules.find(moduleName);
        if(findIt != _modules.end()) {
            return findIt->second;
        }
        return NULL;
    }

    PyObject* GetDict(std::string moduleName) {
        auto findIt = _dictionaries.find(moduleName);
        if(findIt != _dictionaries.end()) {
            return findIt->second;
        }
        return NULL;
    }

    // decref handled internally
    PyObject* LoadModule(std::string moduleName) {
        PyObject* module = GetModule(moduleName);
        if(module)
            return module;

        char* unConstName = const_cast<char*>(moduleName.c_str()); // lol
        // seriously, python's api implementers didn't get the const right?
        module = PyImport_ImportModuleEx(unConstName,
            /*globals*/ NULL, /*locals*/ NULL, /*fromlist*/ NULL);
        if(!module || !PyModule_Check(module)) {
            printf("Error loading module:%s\n", moduleName.c_str());
            PyErr_Print();
            Py_DECREF(module);
            return NULL;
        }
        _modules.insert(std::make_pair(moduleName, module));
        
        PyObject* dict = PyModule_GetDict(module); // borrowed reference
        _dictionaries.insert(std::make_pair(moduleName, dict));
        return module;
    }

    bool CollectNumberList(PyObject* list, PyVisInterface::NumberList& numbers) {
        if(!list || !PyList_Check(list)) {
            return false;
        }

        int numResults = (int)PyList_Size(list);
        numbers.reserve(numbers.size() + numResults);
        for(int i = 0; i < numResults; i++) {
            PyObject* item = PyList_GetItem(list, i); // borrowed ref
            if(!item || !PyFloat_Check(item))
                continue;
            numbers.push_back(PyFloat_AsDouble(item));
        }

        return true;
    }

    void DecRefPythonObjects(HashToPython& hashPyObjs) {
        for(auto obj : hashPyObjs) { // hopefully order doesn't matter
            Py_DECREF(obj.second);
        }
    }

};

static PyVis* g_PyVis = NULL;
bool PyVisInterface::InitPython() {
    if(0 != Py_IsInitialized() || g_PyVis != NULL)
        return true;

    std::unique_ptr<PyVis> pyVis(new PyVis()); 

    Py_Initialize();
    PyErr_Print();

    PyObject* mainModule = pyVis->LoadModule("__main__");
    pyVis->consoleGlobalContextDict = mainModule; 
    if(!mainModule) { return false; }
    PyErr_Print();
    
    // PyRun_SimpleString("print 'Then...'");
    
    // PyObject* mainDict = PyModule_GetDict(mainModule);
    // pyVis->mainDict = mainDict;
    // if(!mainDict || !PyDict_Check(mainDict)) {
    //     printf("Error loading python main dictionary:\n");
    //     PyErr_Print();
    //     return false; 
    // }
    
    PyObject* systemModule = PyImport_ImportModule("sys");
    if(!systemModule || !PyModule_Check(systemModule)) {
        printf("Error loading python module sys:\n");
        PyErr_Print();
        Py_DECREF(systemModule);
        return false;
    }
    PyErr_Print();
    
    PyObject* sysPath = PyObject_GetAttrString(systemModule, "path");
    if(!sysPath || !PyList_Check(sysPath)) {
        printf("Error loading python sys path:\n");
        PyErr_Print();
        Py_DECREF(sysPath);
        return false;
    }
    PyList_Append(sysPath, PyString_FromString("pyvis"));
    PyList_Append(sysPath, PyString_FromString("pyvis/PIMCPy"));
    PyErr_Print();

    std::string scriptName("EmbeddedBinding");
    std::string scriptNameShorthand("eb");
    if(!pyVis->LoadModule(scriptName)) {
        return false;
    }
    PyErr_Print();

    // So far this seems to be necessary to use EmbeddedBinding from the 
    // PyRun_SimpleString context, but they are still referencing the same module
    std::string importToSimple = std::string("import ") + scriptName; 
    PyRun_SimpleString(importToSimple.c_str()); 
    // Same, but now even more convenient! So like "eb.ScriptStep()"
    std::string importToSimpleShorthand = std::string("import ") + scriptName
        + std::string(" as ") + scriptNameShorthand;
    PyRun_SimpleString(importToSimpleShorthand.c_str());
    
    PyRun_SimpleString("import numpy"); // handy, was already loaded above anyway

    PyObject* scriptDict = pyVis->GetDict(scriptName);
    PyRun_String("ScriptCreate()", Py_eval_input, mainModule, scriptDict);
    pyVis->consoleLocalContextDict = scriptDict;

    PyObject* scriptStep = PyDict_GetItemString(scriptDict, "ScriptStep"); // borrowed ref
    pyVis->scriptStep = scriptStep;  // borrowed ref
    if(!scriptStep || !PyCallable_Check(scriptStep)) {
        printf("Error loading python potential step function linkage: no ScriptStep()\n");
        PyErr_Print();
        return false; 
    }

    if(0 == Py_IsInitialized()) {
        return false;
    }

    PyErr_Print();
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
    if(!g_PyVis || !g_PyVis->scriptStep)
        return false;

    PyObject* result = PyObject_CallObject(g_PyVis->scriptStep, /* args */ NULL);
    ScopePyDecRef clearResult(result);
    if(!result || !PyList_Check(result) || PyList_Size(result) <= 0) {
        printf("Python script step didn't return a valid list\n");
        return false;
    }

    PyObject* list = PyList_GetItem(result, 0);
    return g_PyVis->CollectNumberList(list, output);
}

bool PyVisInterface::PathIntegralSingleStep(QuaxolChunk& output) {
    // Timer slowness("python time");
    if(!g_PyVis || !g_PyVis->scriptStep)
        return false;

    PyErr_Print();

    PyObject* result = PyObject_CallObject(g_PyVis->scriptStep, /* args */ NULL);
    ScopePyDecRef clearResult(result);
    if(!result || !PyList_Check(result) || PyList_Size(result) <= 0) {
        printf("Python script step didn't return a valid list\n");
        return false;
    }
    PyObject* list = PyList_GetItem(result, 0);
    PyErr_Print();

    NumberList nums;
    bool success = g_PyVis->CollectNumberList(list, nums);
    // printf("Got %d nums!\n", (int)nums.size());
    QuaxolSpec pos(0,0,0,0);
    for(int i = 0;
            i < (int)nums.size() && i < (int)QuaxolChunk::c_mxSz; 
            i++) {
        double val = nums[i];
        // printf(" val %f ", val); 
        static double max = 10.0f;
        static double min = 0.0f;
        int numFilled = (int)((val - min) / (max - min) * (double)QuaxolChunk::c_mxSz);
        pos.x = i;
        for(int j = 0;
                j < numFilled && j < (int)QuaxolChunk::c_mxSz;
                j++) {
            pos.z = j;
            output.SetAt(pos, true);
        }
    }
    PyErr_Print();

    return success;
}

bool PyVisInterface::RunOneLine(const char* command) {
    // printf("About to try to run '%s'\n", command);
    if(!g_PyVis || !g_PyVis->consoleGlobalContextDict || !g_PyVis->consoleLocalContextDict) {
        printf("RunOneLine failed! dropped '%s'\n", command);
        return false;
    }
    
    // printf("simple:\n");
    int simpleResult = PyRun_SimpleString(command);
    // printf("Result was %d\n", simpleResult);
    PyErr_Print();
    // printf("direct:\n");
    // PyObject* ref = PyRun_String(command, Py_eval_input,
    //     g_PyVis->consoleGlobalContextDict, g_PyVis->consoleLocalContextDict);
    // PyErr_Print();
    // if(ref) {
    //     printf("decref\n");
    //     Py_DECREF(ref);
    // }
    // PyErr_Print();
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
