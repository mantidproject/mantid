#include <boost/python.hpp>

#include "../inc/FrameworkManager.h"
#include "../inc/Workspace.h"
#include "../inc/IAlgorithm.h"
#include "../inc/Algorithm.h"

using namespace boost::python;
using namespace Mantid::Kernel;

//Wrapper for Algorithm class - enables virtual functions to work properly
struct Mantid_Algorithm_Wrapper: Algorithm
{
    Mantid_Algorithm_Wrapper(PyObject* py_self_):
        Algorithm(), py_self(py_self_) {}

    const std::string& name() const {
        return call_method< const std::string& >(py_self, "name");
    }

    const std::string& default_name() const {
        return Algorithm::name();
    }

    const std::string& version() const {
        return call_method< const std::string& >(py_self, "version");
    }

    const std::string& default_version() const {
        return Algorithm::version();
    }

    StatusCode initialize() {
        return call_method< StatusCode >(py_self, "initialize");
    }

    StatusCode default_initialize() {
        return Algorithm::initialize();
    }

    StatusCode execute() {
        return call_method< StatusCode >(py_self, "execute");
    }

    StatusCode default_execute() {
        return Algorithm::execute();
    }

    StatusCode finalize() {
        return call_method< StatusCode >(py_self, "finalize");
    }

    StatusCode default_finalize() {
        return Algorithm::finalize();
    }

    bool isInitialized() const {
        return call_method< bool >(py_self, "isInitialized");
    }

    bool default_isInitialized() const {
        return Algorithm::isInitialized();
    }

    bool isExecuted() const {
        return call_method< bool >(py_self, "isExecuted");
    }

    bool default_isExecuted() const {
        return Algorithm::isExecuted();
    }

    bool isFinalized() const {
        return call_method< bool >(py_self, "isFinalized");
    }

    bool default_isFinalized() const {
        return Algorithm::isFinalized();
    }

    StatusCode setProperty(const std::string& p0) {
        return call_method< StatusCode >(py_self, "setProperty", p0);
    }

    StatusCode default_setProperty(const std::string& p0) {
        return Algorithm::setProperty(p0);
    }

    StatusCode setProperty(const std::string& p0, const std::string& p1) {
        return call_method< StatusCode >(py_self, "setProperty", p0, p1);
    }

    StatusCode default_setProperty(const std::string& p0, const std::string& p1) {
        return Algorithm::setProperty(p0, p1);
    }

    StatusCode getProperty(const std::string& p0, std::string& p1) const {
        return call_method< StatusCode >(py_self, "getProperty", p0, p1);
    }

    StatusCode default_getProperty(const std::string& p0, std::string& p1) const {
        return Algorithm::getProperty(p0, p1);
    }

    StatusCode init() {
        return call_method< StatusCode >(py_self, "init");
    }

    StatusCode exec() {
        return call_method< StatusCode >(py_self, "exec");
    }

    StatusCode final() {
        return call_method< StatusCode >(py_self, "final");
    }

    PyObject* py_self;
};

struct Mantid_Workspace_Wrapper: Workspace
{
    const std::string id() const {
        return call_method< const std::string >(py_self, "id");
    }

    long int getMemorySize() const {
        return call_method< long int >(py_self, "getMemorySize");
    }

    long int default_getMemorySize() const {
        return Workspace::getMemorySize();
    }

    PyObject* py_self;
};



//Definitions
#if _WIN32
BOOST_PYTHON_MODULE(MantidKernel)
{
#else
BOOST_PYTHON_MODULE(libMantidKernel)
{
#endif	
    class_<FrameworkManager >("FrameworkManager", init<  >())
        .def(init< const FrameworkManager& >())
        .def("initialize", &FrameworkManager::initialize)
        .def("createAlgorithm", (IAlgorithm* (FrameworkManager::*)(const std::string&) )&FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("createAlgorithm", (IAlgorithm* (FrameworkManager::*)(const std::string&, const std::string&) )&FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("exec", &FrameworkManager::exec, return_value_policy< manage_new_object >())
        .def("getWorkspace", &FrameworkManager::getWorkspace, return_value_policy< manage_new_object >())
    ;
    class_< Algorithm, boost::noncopyable, Mantid_Algorithm_Wrapper >("Algorithm", init<  >())
        .def("name", (const std::string& (Algorithm::*)() const)&Algorithm::name, (const std::string& (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_name, return_value_policy< copy_const_reference >())
        .def("version", (const std::string& (Algorithm::*)() const)&Algorithm::version, (const std::string& (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_version, return_value_policy< copy_const_reference >())
        .def("initialize", (StatusCode (Algorithm::*)() )&Algorithm::initialize, (StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_initialize)
        .def("execute", (StatusCode (Algorithm::*)() )&Algorithm::execute, (StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_execute)
        .def("finalize", (StatusCode (Algorithm::*)() )&Algorithm::finalize, (StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_finalize)
        .def("isInitialized", (bool (Algorithm::*)() const)&Algorithm::isInitialized, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isInitialized)
        .def("isExecuted", (bool (Algorithm::*)() const)&Algorithm::isExecuted, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isExecuted)
        .def("isFinalized", (bool (Algorithm::*)() const)&Algorithm::isFinalized, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isFinalized)
        .def("setProperty", (StatusCode (Algorithm::*)(const std::string&) )&Algorithm::setProperty, (StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&))&Mantid_Algorithm_Wrapper::default_setProperty)
        .def("setProperty", (StatusCode (Algorithm::*)(const std::string&, const std::string&) )&Algorithm::setProperty, (StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&, const std::string&))&Mantid_Algorithm_Wrapper::default_setProperty)
        .def("getProperty", (StatusCode (Algorithm::*)(const std::string&, std::string&) const)&Algorithm::getProperty, (StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&, std::string&) const)&Mantid_Algorithm_Wrapper::default_getProperty)
        //.def("createSubAlgorithm", &Algorithm::createSubAlgorithm)
        .def("subAlgorithms", &Algorithm::subAlgorithms, return_value_policy< manage_new_object >())
    ;
    class_< Workspace, boost::noncopyable, Mantid_Workspace_Wrapper >("Workspace", no_init)
        .def("id", pure_virtual(&Workspace::id))
        .def("getMemorySize", &Workspace::getMemorySize, &Mantid_Workspace_Wrapper::default_getMemorySize)
        .def("setTitle", &Workspace::setTitle)
        .def("setComment", &Workspace::setComment)
        .def("getComment", &Workspace::getComment)
        .def("getTitle", &Workspace::getTitle)
    ;
}


