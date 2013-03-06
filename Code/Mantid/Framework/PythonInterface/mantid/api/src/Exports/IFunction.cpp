#include "MantidAPI/IFunction.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmWrapper.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

// Python frameobject. This is under the boost includes so that boost will have done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using Mantid::API::IFunction;
using namespace boost::python;

namespace
{
  ///@cond

  //------------------------------------------------------------------------------------------------------
  /**
   * A Python friendly version that returns the registered functions as a list.
   * @param self :: Enables it to be called as a member function on the FunctionFactory class
   */
  PyObject * getCategories(IFunction & self)
  {
    std::vector<std::string> categories = self.categories();

    PyObject *registered = PyList_New(0);
    for (auto category = categories.begin(); category != categories.end(); ++category)
    {
      PyObject *value = PyString_FromString(category->c_str());
      if (PyList_Append(registered, value))
        throw std::runtime_error("Failed to insert value into PyList");
    }

    return registered;
  }
  ///@endcond
}

void export_IFunction()
{

  REGISTER_SHARED_PTR_TO_PYTHON(IFunction);
  class_<IFunction, boost::noncopyable>("IFunction", "Base-class for C IFunctions", no_init);

  class_<IFunction,boost::noncopyable>("IFunction", no_init)
      .def("name", &IFunction::name, "Return the name of the function")
      .def("__repr__", &IFunction::asString, "Return a string representation of the function")
      .def("categories", &getCategories, "Returns a list of the categories for an algorithm")
      .def("numParams", &IFunction::nParams, "Return the number of parameters")
      .def("getParamName", &IFunction::parameterName, "Return the name of the ith parameter")
      .def("getParamDescr", &IFunction::parameterDescription, "Return a description of the ith parameter")
      .def("getParamExplicit", &IFunction::isExplicitlySet,
           "Return whether the ith parameter needs to be explicitely set")
      .def("getParamValue", (double (IFunction::*)(std::size_t) const)&IFunction::getParameter,
           "Get the value of the ith parameter")
    ;

}
