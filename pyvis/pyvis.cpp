#include "pyvis.h" // has FD_USE_PYTHON_HOOK 

#ifdef FD_USE_PYTHON_HOOK

#include <python2.7/Python.h>
#include "../common/timer.h"


namespace fd {

class PyVis {
    public:
    PyObject* mainDict;

};

static PyVis* g_sPyVis = NULL;

bool PathIntegralSingleStep() {
    static double sLastElapsed;
    static Timer sPyPerf;
    Timer huh("python time");
    sPyPerf.Start();

    if(!g_sPyVis) {
        g_sPyVis = new PyVis();
        Py_Initialize();
    } 
    if(0 == Py_IsInitialized())
        return false;


    // // Put the current dir on the path to load a local module
    PyObject* systemModule = PyImport_ImportModule("sys");
    PyObject* sysPath = PyObject_GetAttrString(systemModule, "path");
    PyList_Append(sysPath, PyString_FromString("pyvis/PIMCPy"));

    // PyObject* mainModule = PyImport_AddModule("__main__");
    // PyObject* mainDict = PyModule_GetDict(mainModule);
    

    // PyRun_String("lastPythonTime = 0.0", /*startToken*/ 0, mainDict, mainDict);
    // PyObject* newValue = PyFloat_FromDouble(sLastElapsed);
    // lastPyhonTy

    //PyObject* pimcModule = PyImport_ImportModule("TestSingleSlicePotential");
    PyRun_SimpleString("import TestSingleSlicePotential");
    PyRun_SimpleString("import PIMC");
    PyRun_SimpleString("import numpy");
    
    PyRun_SimpleString("print 'sqrt is %f' % (numpy.sqrt(13))");
    //PyRun_SimpleString("TestSingleSlicePotential.CalcSingleSlicePotential()");

    // char currentTime[256];
    // snprintf(currentTime, sizeof(currentTime) - 1, "lastPythonTime = %f", sLastElapsed);
    // currentTime[sizeof(currentTime) - 1] = '\0'; // seriously?
    // std::string terriblePassing(currentTime);
    // PyRun_SimpleString(terriblePassing.c_str());

    // PyRun_SimpleString("print('fuck you %d %f' % (13, 1.0/3))");


    // Py_Finalize(); // lol

    sLastElapsed = sPyPerf.GetElapsed();
    return true;
}

bool HereIsWhereWeWouldLikeWriteSomeTestCodeCommaTotally() {
   static double sLastElapsed;
    static Timer sPyPerf;
    Timer huh("python time");
    sPyPerf.Start();

    if(!g_sPyVis) {
        g_sPyVis = new PyVis();
        Py_Initialize();
    } 
    if(0 == Py_IsInitialized())
        return false;


    // // Put the current dir on the path to load a local module
    PyObject* systemModule = PyImport_ImportModule("sys");
    PyObject* sysPath = PyObject_GetAttrString(systemModule, "path");
    PyList_Append(sysPath, PyString_FromString("pyvis"));

    PyObject* mainModule = PyImport_AddModule("__main__");
    PyObject* mainDict = PyModule_GetDict(mainModule);
    

    // PyRun_String("lastPythonTime = 0.0", /*startToken*/ 0, mainDict, mainDict);
    // PyObject* newValue = PyFloat_FromDouble(sLastElapsed);
    // lastPyhonTy

    // PyObject* pimcModule = PyImport_ImportModule("testpy");
    PyRun_SimpleString("import testpy");
    PyRun_SimpleString("import numpy");
    
    PyRun_SimpleString("lastPythonTime = 0.0");
    PyRun_SimpleString("print 'sqrt is %f' % (numpy.sqrt(13))");
    PyRun_SimpleString("print('PRE FUNC lastPythonTime is %f' % (lastPythonTime))");
    PyRun_SimpleString("testpy.FunctionInTestPy()");
    PyRun_SimpleString("print('AFTER lastPythonTime is %f' % (lastPythonTime))");
    
    // char currentTime[256];
    // snprintf(currentTime, sizeof(currentTime) - 1, "lastPythonTime = %f", sLastElapsed);
    // currentTime[sizeof(currentTime) - 1] = '\0'; // seriously?
    // std::string terriblePassing(currentTime);
    // PyRun_SimpleString(terriblePassing.c_str());

    // PyRun_SimpleString("print('fuck you %d %f' % (13, 1.0/3))");


    // Py_Finalize(); // lol

    sLastElapsed = sPyPerf.GetElapsed();
    return true;
}

} // namespace fd

#endif //def FD_USE_PYTHON_HOOK
