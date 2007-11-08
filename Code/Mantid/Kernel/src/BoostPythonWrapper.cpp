
// Boost Includes ==============================================================
#include <boost/python.hpp>
#include <boost/cstdint.hpp>

// Includes ====================================================================
#include "Algorithm.h"
#include "FrameworkManager.h"
#include "Workspace.h"

// Using =======================================================================
using namespace boost::python;

// Declarations ================================================================
namespace  {

struct Mantid_Kernel_Algorithm_Wrapper: Mantid::Kernel::Algorithm
{
    Mantid_Kernel_Algorithm_Wrapper(PyObject* py_self_):
        Mantid::Kernel::Algorithm(), py_self(py_self_) {}

    const std::string& name() const {
        return call_method< const std::string& >(py_self, "name");
    }

    const std::string& default_name() const {
        return Mantid::Kernel::Algorithm::name();
    }

    const std::string& version() const {
        return call_method< const std::string& >(py_self, "version");
    }

    const std::string& default_version() const {
        return Mantid::Kernel::Algorithm::version();
    }

    Mantid::Kernel::StatusCode initialize() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "initialize");
    }

    Mantid::Kernel::StatusCode default_initialize() {
        return Mantid::Kernel::Algorithm::initialize();
    }

    Mantid::Kernel::StatusCode execute() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "execute");
    }

    Mantid::Kernel::StatusCode default_execute() {
        return Mantid::Kernel::Algorithm::execute();
    }

    Mantid::Kernel::StatusCode finalize() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "finalize");
    }

    Mantid::Kernel::StatusCode default_finalize() {
        return Mantid::Kernel::Algorithm::finalize();
    }

    bool isInitialized() const {
        return call_method< bool >(py_self, "isInitialized");
    }

    bool default_isInitialized() const {
        return Mantid::Kernel::Algorithm::isInitialized();
    }

    bool isExecuted() const {
        return call_method< bool >(py_self, "isExecuted");
    }

    bool default_isExecuted() const {
        return Mantid::Kernel::Algorithm::isExecuted();
    }

    bool isFinalized() const {
        return call_method< bool >(py_self, "isFinalized");
    }

    bool default_isFinalized() const {
        return Mantid::Kernel::Algorithm::isFinalized();
    }

    Mantid::Kernel::StatusCode setProperty(const std::string& p0) {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "setProperty", p0);
    }

    Mantid::Kernel::StatusCode default_setProperty(const std::string& p0) {
        return Mantid::Kernel::Algorithm::setProperty(p0);
    }

    Mantid::Kernel::StatusCode setProperty(const std::string& p0, const std::string& p1) {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "setProperty", p0, p1);
    }

    Mantid::Kernel::StatusCode default_setProperty(const std::string& p0, const std::string& p1) {
        return Mantid::Kernel::Algorithm::setProperty(p0, p1);
    }

    Mantid::Kernel::StatusCode getProperty(const std::string& p0, std::string& p1) const {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "getProperty", p0, p1);
    }

    Mantid::Kernel::StatusCode default_getProperty(const std::string& p0, std::string& p1) const {
        return Mantid::Kernel::Algorithm::getProperty(p0, p1);
    }

    Mantid::Kernel::StatusCode init() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "init");
    }

    Mantid::Kernel::StatusCode exec() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "exec");
    }

    Mantid::Kernel::StatusCode final() {
        return call_method< Mantid::Kernel::StatusCode >(py_self, "final");
    }

    PyObject* py_self;
};

struct Mantid_Kernel_Workspace_Wrapper: Mantid::Kernel::Workspace
{
    const std::string id() const {
        return call_method< const std::string >(py_self, "id");
    }

    long int getMemorySize() const {
        return call_method< long int >(py_self, "getMemorySize");
    }

    long int default_getMemorySize() const {
        return Mantid::Kernel::Workspace::getMemorySize();
    }

    PyObject* py_self;
};


}// namespace 


// Module ======================================================================
#if _WIN32
BOOST_PYTHON_MODULE(MantidKernel)
{
#else
BOOST_PYTHON_MODULE(libMantidKernel)
{
#endif	
    class_< Mantid::Kernel::Algorithm, boost::noncopyable, Mantid_Kernel_Algorithm_Wrapper >("Algorithm", init<  >())
        .def("name", (const std::string& (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::name, (const std::string& (Mantid_Kernel_Algorithm_Wrapper::*)() const)&Mantid_Kernel_Algorithm_Wrapper::default_name, return_value_policy< copy_const_reference >())
        .def("version", (const std::string& (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::version, (const std::string& (Mantid_Kernel_Algorithm_Wrapper::*)() const)&Mantid_Kernel_Algorithm_Wrapper::default_version, return_value_policy< copy_const_reference >())
        .def("initialize", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)() )&Mantid::Kernel::Algorithm::initialize, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)())&Mantid_Kernel_Algorithm_Wrapper::default_initialize)
        .def("execute", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)() )&Mantid::Kernel::Algorithm::execute, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)())&Mantid_Kernel_Algorithm_Wrapper::default_execute)
        .def("finalize", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)() )&Mantid::Kernel::Algorithm::finalize, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)())&Mantid_Kernel_Algorithm_Wrapper::default_finalize)
        .def("isInitialized", (bool (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::isInitialized, (bool (Mantid_Kernel_Algorithm_Wrapper::*)() const)&Mantid_Kernel_Algorithm_Wrapper::default_isInitialized)
        .def("isExecuted", (bool (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::isExecuted, (bool (Mantid_Kernel_Algorithm_Wrapper::*)() const)&Mantid_Kernel_Algorithm_Wrapper::default_isExecuted)
        .def("isFinalized", (bool (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::isFinalized, (bool (Mantid_Kernel_Algorithm_Wrapper::*)() const)&Mantid_Kernel_Algorithm_Wrapper::default_isFinalized)
        .def("setProperty", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)(const std::string&) )&Mantid::Kernel::Algorithm::setProperty, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)(const std::string&))&Mantid_Kernel_Algorithm_Wrapper::default_setProperty)
        .def("setProperty", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)(const std::string&, const std::string&) )&Mantid::Kernel::Algorithm::setProperty, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)(const std::string&, const std::string&))&Mantid_Kernel_Algorithm_Wrapper::default_setProperty)
        .def("getProperty", (Mantid::Kernel::StatusCode (Mantid::Kernel::Algorithm::*)(const std::string&, std::string&) const)&Mantid::Kernel::Algorithm::getProperty, (Mantid::Kernel::StatusCode (Mantid_Kernel_Algorithm_Wrapper::*)(const std::string&, std::string&) const)&Mantid_Kernel_Algorithm_Wrapper::default_getProperty)
//        .def("createSubAlgorithm", &Mantid::Kernel::Algorithm::createSubAlgorithm)
//        .def("subAlgorithms", (const std::vector<Mantid::Kernel::Algorithm*,std::allocator<Mantid::Kernel::Algorithm*> >& (Mantid::Kernel::Algorithm::*)() const)&Mantid::Kernel::Algorithm::subAlgorithms, return_value_policy< copy_const_reference >())
//        .def("subAlgorithms", (std::vector<Mantid::Kernel::Algorithm*,std::allocator<Mantid::Kernel::Algorithm*> >& (Mantid::Kernel::Algorithm::*)() )&Mantid::Kernel::Algorithm::subAlgorithms, return_value_policy< copy_const_reference >())
    ;

    class_< Mantid::Kernel::FrameworkManager >("FrameworkManager", init<  >())
        .def(init< const Mantid::Kernel::FrameworkManager& >())
        .def("initialize", &Mantid::Kernel::FrameworkManager::initialize)
        .def("clear", &Mantid::Kernel::FrameworkManager::clear)
        .def("createAlgorithm", (Mantid::Kernel::IAlgorithm* (Mantid::Kernel::FrameworkManager::*)(const std::string&) )&Mantid::Kernel::FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("createAlgorithm", (Mantid::Kernel::IAlgorithm* (Mantid::Kernel::FrameworkManager::*)(const std::string&, const std::string&) )&Mantid::Kernel::FrameworkManager::createAlgorithm, return_value_policy< manage_new_object >())
        .def("exec", &Mantid::Kernel::FrameworkManager::exec, return_value_policy< manage_new_object >())
        .def("getWorkspace", &Mantid::Kernel::FrameworkManager::getWorkspace, return_value_policy< manage_new_object >())
    ;

    class_< Mantid::Kernel::Workspace, boost::noncopyable, Mantid_Kernel_Workspace_Wrapper >("Workspace", no_init)
        .def("id", pure_virtual(&Mantid::Kernel::Workspace::id))
        .def("getMemorySize", &Mantid::Kernel::Workspace::getMemorySize, &Mantid_Kernel_Workspace_Wrapper::default_getMemorySize)
        .def("setTitle", &Mantid::Kernel::Workspace::setTitle)
        .def("setComment", &Mantid::Kernel::Workspace::setComment)
        .def("getComment", &Mantid::Kernel::Workspace::getComment, return_value_policy< copy_const_reference >())
        .def("getTitle", &Mantid::Kernel::Workspace::getTitle, return_value_policy< copy_const_reference >())
    ;

}

