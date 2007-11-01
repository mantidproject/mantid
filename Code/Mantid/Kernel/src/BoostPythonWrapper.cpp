#include <boost/python.hpp>

#include "../inc/FrameworkManager.h"
#include "../inc/Workspace.h"
#include "../inc/IAlgorithm.h"
#include "../inc/Algorithm.h"

using namespace boost::python;

//Wrapper for Algorithm class - enables virtual functions to work properly
struct Mantid_Algorithm_Wrapper: Mantid::Algorithm
{
    Mantid_Algorithm_Wrapper(PyObject* py_self_):
        Mantid::Algorithm(), py_self(py_self_) {}

    const std::string& name() const {
        return call_method< const std::string& >(py_self, "name");
    }

    const std::string& default_name() const {
        return Mantid::Algorithm::name();
    }

    const std::string& version() const {
        return call_method< const std::string& >(py_self, "version");
    }

    const std::string& default_version() const {
        return Mantid::Algorithm::version();
    }

    Mantid::StatusCode initialize() {
        return call_method< Mantid::StatusCode >(py_self, "initialize");
    }

    Mantid::StatusCode default_initialize() {
        return Mantid::Algorithm::initialize();
    }

    Mantid::StatusCode execute() {
        return call_method< Mantid::StatusCode >(py_self, "execute");
    }

    Mantid::StatusCode default_execute() {
        return Mantid::Algorithm::execute();
    }

    Mantid::StatusCode finalize() {
        return call_method< Mantid::StatusCode >(py_self, "finalize");
    }

    Mantid::StatusCode default_finalize() {
        return Mantid::Algorithm::finalize();
    }

    bool isInitialized() const {
        return call_method< bool >(py_self, "isInitialized");
    }

    bool default_isInitialized() const {
        return Mantid::Algorithm::isInitialized();
    }

    bool isExecuted() const {
        return call_method< bool >(py_self, "isExecuted");
    }

    bool default_isExecuted() const {
        return Mantid::Algorithm::isExecuted();
    }

    bool isFinalized() const {
        return call_method< bool >(py_self, "isFinalized");
    }

    bool default_isFinalized() const {
        return Mantid::Algorithm::isFinalized();
    }

    Mantid::StatusCode setProperty(const std::string& p0) {
        return call_method< Mantid::StatusCode >(py_self, "setProperty", p0);
    }

    Mantid::StatusCode default_setProperty(const std::string& p0) {
        return Mantid::Algorithm::setProperty(p0);
    }

    Mantid::StatusCode setProperty(const std::string& p0, const std::string& p1) {
        return call_method< Mantid::StatusCode >(py_self, "setProperty", p0, p1);
    }

    Mantid::StatusCode default_setProperty(const std::string& p0, const std::string& p1) {
        return Mantid::Algorithm::setProperty(p0, p1);
    }

    Mantid::StatusCode getProperty(const std::string& p0, std::string& p1) const {
        return call_method< Mantid::StatusCode >(py_self, "getProperty", p0, p1);
    }

    Mantid::StatusCode default_getProperty(const std::string& p0, std::string& p1) const {
        return Mantid::Algorithm::getProperty(p0, p1);
    }

    Mantid::StatusCode init() {
        return call_method< Mantid::StatusCode >(py_self, "init");
    }

    Mantid::StatusCode exec() {
        return call_method< Mantid::StatusCode >(py_self, "exec");
    }

    Mantid::StatusCode final() {
        return call_method< Mantid::StatusCode >(py_self, "final");
    }

    PyObject* py_self;
};

struct Mantid_Workspace_Wrapper: Mantid::Workspace
{
    const std::string id() const {
        return call_method< const std::string >(py_self, "id");
    }

    long int getMemorySize() const {
        return call_method< long int >(py_self, "getMemorySize");
    }

    long int default_getMemorySize() const {
        return Mantid::Workspace::getMemorySize();
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
    class_< Mantid::FrameworkManager >("FrameworkManager", init<  >())
        .def(init< const Mantid::FrameworkManager& >())
        .def("initialize", &Mantid::FrameworkManager::initialize)
        .def("createAlgorithm", (Mantid::IAlgorithm* (Mantid::FrameworkManager::*)(const std::string&) )&Mantid::FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("createAlgorithm", (Mantid::IAlgorithm* (Mantid::FrameworkManager::*)(const std::string&, const std::string&) )&Mantid::FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("exec", &Mantid::FrameworkManager::exec, return_value_policy< manage_new_object >())
        .def("getWorkspace", &Mantid::FrameworkManager::getWorkspace, return_value_policy< manage_new_object >())
    ;
    class_< Mantid::Algorithm, boost::noncopyable, Mantid_Algorithm_Wrapper >("Algorithm", init<  >())
        .def("name", (const std::string& (Mantid::Algorithm::*)() const)&Mantid::Algorithm::name, (const std::string& (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_name, return_value_policy< copy_const_reference >())
        .def("version", (const std::string& (Mantid::Algorithm::*)() const)&Mantid::Algorithm::version, (const std::string& (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_version, return_value_policy< copy_const_reference >())
        .def("initialize", (Mantid::StatusCode (Mantid::Algorithm::*)() )&Mantid::Algorithm::initialize, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_initialize)
        .def("execute", (Mantid::StatusCode (Mantid::Algorithm::*)() )&Mantid::Algorithm::execute, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_execute)
        .def("finalize", (Mantid::StatusCode (Mantid::Algorithm::*)() )&Mantid::Algorithm::finalize, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)())&Mantid_Algorithm_Wrapper::default_finalize)
        .def("isInitialized", (bool (Mantid::Algorithm::*)() const)&Mantid::Algorithm::isInitialized, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isInitialized)
        .def("isExecuted", (bool (Mantid::Algorithm::*)() const)&Mantid::Algorithm::isExecuted, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isExecuted)
        .def("isFinalized", (bool (Mantid::Algorithm::*)() const)&Mantid::Algorithm::isFinalized, (bool (Mantid_Algorithm_Wrapper::*)() const)&Mantid_Algorithm_Wrapper::default_isFinalized)
        .def("setProperty", (Mantid::StatusCode (Mantid::Algorithm::*)(const std::string&) )&Mantid::Algorithm::setProperty, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&))&Mantid_Algorithm_Wrapper::default_setProperty)
        .def("setProperty", (Mantid::StatusCode (Mantid::Algorithm::*)(const std::string&, const std::string&) )&Mantid::Algorithm::setProperty, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&, const std::string&))&Mantid_Algorithm_Wrapper::default_setProperty)
        .def("getProperty", (Mantid::StatusCode (Mantid::Algorithm::*)(const std::string&, std::string&) const)&Mantid::Algorithm::getProperty, (Mantid::StatusCode (Mantid_Algorithm_Wrapper::*)(const std::string&, std::string&) const)&Mantid_Algorithm_Wrapper::default_getProperty)
        //.def("createSubAlgorithm", &Mantid::Algorithm::createSubAlgorithm)
        .def("subAlgorithms", &Mantid::Algorithm::subAlgorithms, return_value_policy< manage_new_object >())
    ;
    class_< Mantid::Workspace, boost::noncopyable, Mantid_Workspace_Wrapper >("Workspace", no_init)
        .def("id", pure_virtual(&Mantid::Workspace::id))
        .def("getMemorySize", &Mantid::Workspace::getMemorySize, &Mantid_Workspace_Wrapper::default_getMemorySize)
        .def("setTitle", &Mantid::Workspace::setTitle)
        .def("setComment", &Mantid::Workspace::setComment)
        .def("getComment", &Mantid::Workspace::getComment)
        .def("getTitle", &Mantid::Workspace::getTitle)
    ;
}


