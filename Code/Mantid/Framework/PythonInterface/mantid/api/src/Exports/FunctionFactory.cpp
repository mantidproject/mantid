#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::FunctionFactoryImpl;
using Mantid::API::FunctionFactory;
using Mantid::API::IFunction;
using Mantid::PythonInterface::PythonObjectInstantiator;
using Mantid::Kernel::AbstractInstantiator;

using namespace boost::python;

namespace
{
  ///@cond

  //------------------------------------------------------------------------------------------------------
  /**
   * A Python friendly version that returns the registered functions as a list.
   * @param self :: Enables it to be called as a member function on the FunctionFactory class
   */
  PyObject * getFunctionNames(FunctionFactoryImpl & self)
  {
    const std::vector<std::string>& names = self.getFunctionNames<Mantid::API::IFunction>();

    PyObject *registered = PyList_New(0);
    for (auto name = names.begin(); name != names.end(); ++name)
    {
      PyObject *value = PyString_FromString(name->c_str());
      if (PyList_Append(registered, value))
        throw std::runtime_error("Failed to insert value into PyList");
    }

    return registered;
  }

  //--------------------------------------------- Function registration ------------------------------------------------

    /// Python algorithm registration mutex in anonymous namespace (aka static)
    Poco::Mutex FUNCTION_REGISTER_MUTEX;

    /**
     * A free function to register a fit function from Python
     * @param obj :: A Python object that should either be a class type derived from IFunction
     *              or an instance of a class type derived from IFunction
     */
    void subscribe(FunctionFactoryImpl & self, const boost::python::object & obj )
    {
      Poco::ScopedLock<Poco::Mutex> lock(FUNCTION_REGISTER_MUTEX);
      static PyTypeObject * baseClass =
          const_cast<PyTypeObject*>(converter::registered<IFunction>::converters.to_python_target_type());

      // obj could be or instance/class, check instance first
      PyObject *classObject(NULL);
      if( PyObject_IsInstance(obj.ptr(), (PyObject*)baseClass) )
      {
        classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
      }
      else if(PyObject_IsSubclass(obj.ptr(), (PyObject*)baseClass))
      {
        classObject = obj.ptr(); // We need to ensure the type of lifetime management so grab the raw pointer
      }
      else
      {
        throw std::invalid_argument("Cannot register a function that does not derive from IFunction.");
      }
      //Instantiator will store a reference to the class object, so increase reference count with borrowed template
      auto classHandle = handle<>(borrowed(classObject));
      auto *creator =  new PythonObjectInstantiator<IFunction>(object(classHandle));

      // Find the function name
      auto func = creator->createInstance();

      // Takes ownership of instantiator
      self.subscribe(func->name(), creator, FunctionFactoryImpl::OverwriteCurrent);
    }
  ///@endcond
}

void export_FunctionFactory()
{

  class_<FunctionFactoryImpl,boost::noncopyable>("FunctionFactoryImpl", no_init)
      .def("getFunctionNames", &getFunctionNames,
           "Returns a list of the currently available functions")
      .def("createFunction", &FunctionFactoryImpl::createFunction,
           "Return a pointer to the requested function")
      .def("subscribe", &subscribe, "Register a Python class derived from IFunction into the factory")
      .def("unsubscribe", &FunctionFactoryImpl::unsubscribe, "Remove a type from the factory")

      .def("Instance", &FunctionFactory::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the FunctionFactory singleton")
      .staticmethod("Instance")
    ;

}
