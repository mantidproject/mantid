#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>

using Mantid::API::IFunction;
using Mantid::PythonInterface::IFunctionAdapter;
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
  // -- Declare property overloads --
  // The usual BOOST_MEMBER_FUNCTION_OVERLOADS doesn't work using the wrapper held type

  // declareProperty(name)
  typedef void(IFunctionAdapter::*declareParameterType1)(const std::string &);
  // declareProperty(name,defaultValue)
  typedef void(IFunctionAdapter::*declareParameterType2)(const std::string &,double);
  // declareProperty(name,defaultValue, description)
  typedef void(IFunctionAdapter::*declareParameterType3)(const std::string &,double,const std::string &);

  ///@endcond
}

void export_IFunction()
{

  REGISTER_SHARED_PTR_TO_PYTHON(IFunction);

  class_<IFunction, IFunctionAdapter, boost::noncopyable>("IFunction", "Base class for all functions", no_init)
    .def("name", &IFunction::name, "Return the name of the function")

    .def("initialize", &IFunction::initialize, "Declares any parameters and attributes on the function")

    .def("getCategories", &getCategories, "Returns a list of the categories for an algorithm")

    .def("nAttributes", &IFunction::nAttributes, "Return the number of attributes (non-fitting arguments)")

    .def("nParams", &IFunction::nParams, "Return the number of parameters")

    .def("parameterName", &IFunction::parameterName, "Return the name of the ith parameter")

    .def("paramDescription", &IFunction::parameterDescription, "Return a description of the ith parameter")

    .def("isExplicitlySet", &IFunction::isExplicitlySet,
         "Return whether the ith parameter needs to be explicitely set")

    .def("getParameterValue", (double (IFunction::*)(size_t) const)&IFunction::getParameter,
         "Get the value of the ith parameter")

    .def("setParameter", (void (IFunction::*)(size_t,const double &,bool))&IFunction::setParameter,
         "Set the value of the ith parameter")

    .def("declareAttribute", &IFunctionAdapter::declareAttribute, "Declare an attribute with an initial value")

    .def("getAttributeValue", &IFunctionAdapter::getAttributeValue, "Return the value of the named attribute")

    .def("declareParameter", &IFunctionAdapter::declareFitParameter,
          "Declare a fitting parameter settings its default value & description")

    .def("declareParameter", &IFunctionAdapter::declareFitParameterNoDescr,
         "Declare a fitting parameter settings its default value")

    .def("declareParameter", &IFunctionAdapter::declareFitParameterZeroInit,
         "Declare a fitting parameter settings its default value to 0.0")

    //-- Deprecated functions that have the wrong names --
    .def("categories", &getCategories, "Returns a list of the categories for an algorithm")
    .def("numParams", &IFunction::nParams, "Return the number of parameters")
    .def("getParamName", &IFunction::parameterName, "Return the name of the ith parameter")
    .def("getParamDescr", &IFunction::parameterDescription, "Return a description of the ith parameter")
    .def("getParamExplicit", &IFunction::isExplicitlySet,
         "Return whether the ith parameter needs to be explicitely set")
    .def("getParamValue", (double (IFunction::*)(std::size_t) const)&IFunction::getParameter,
         "Get the value of the ith parameter")
    //-- Python special methods --
    .def("__repr__", &IFunction::asString, "Return a string representation of the function")
    ;

}
