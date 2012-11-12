#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::FunctionFactoryImpl;
using Mantid::API::FunctionFactory;
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
    std::vector<std::string> names = self.getFunctionNames<Mantid::API::IFunction>();

    PyObject *registered = PyList_New(0);
    for (auto name = names.begin(); name != names.end(); ++name)
    {
      PyObject *value = PyString_FromString(name->c_str());
      if (PyList_Append(registered, value))
        throw std::runtime_error("Failed to insert value into PyList");
    }

    return registered;
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
      .def("Instance", &FunctionFactory::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the FunctionFactory singleton")
      .staticmethod("Instance")
    ;

}
