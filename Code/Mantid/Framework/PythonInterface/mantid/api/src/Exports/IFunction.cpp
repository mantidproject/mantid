#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

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
  // -- Set property overloads --
  // setProperty(index,value,explicit)
  typedef void(IFunction::*setParameterType1)(size_t,const double & value,bool);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType1_Overloads, setParameter, 2, 3)
  // setProperty(index,value,explicit)
  typedef void(IFunction::*setParameterType2)(const std::string &,const double & value,bool);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads, setParameter, 2, 3)


  ///@endcond
}

void export_IFunction()
{

  register_ptr_to_python<boost::shared_ptr<IFunction>>();

  class_<IFunction, IFunctionAdapter, boost::noncopyable>("IFunction", "Base class for all functions", no_init)
    .def("name", &IFunction::name, "Return the name of the function")

    .def("category", &IFunctionAdapter::category, "Return a semi-colon(;) separated string for the categories this class should belong to. For sub-categories use a \\ separator")

    .def("initialize", &IFunction::initialize, "Declares any parameters and attributes on the function")

    .def("getCategories", &getCategories, "Returns a list of the categories for an algorithm")

    .def("nAttributes", &IFunction::nAttributes, "Return the number of attributes (non-fitting arguments)")

    .def("attributeNames", &IFunction::getAttributeNames, "The names of all the attributes")

    .def("nParams", &IFunction::nParams, "Return the number of parameters")

    .def("parameterName", &IFunction::parameterName, "Return the name of the ith parameter")

    .def("paramDescription", &IFunction::parameterDescription, "Return a description of the ith parameter")

    .def("isExplicitlySet", &IFunction::isExplicitlySet,
         "Return whether the ith parameter needs to be explicitely set")

    .def("getParameterValue", (double (IFunction::*)(size_t) const)&IFunction::getParameter,
         "Get the value of the ith parameter")

    .def("getParameterValue", (double (IFunction::*)(const std::string&) const)&IFunction::getParameter,
         "Get the value of the named parameter")

    .def("setParameter", (setParameterType1)&IFunction::setParameter,
         setParameterType1_Overloads("Sets the value of the ith parameter"))

    .def("setParameter", (setParameterType2)&IFunction::setParameter,
         setParameterType2_Overloads("Sets the value of the named parameter"))

    .def("declareAttribute", &IFunctionAdapter::declareAttribute, "Declare an attribute with an initial value")

    .def("getAttributeValue", (PyObject * (IFunctionAdapter::*)(const std::string &))&IFunctionAdapter::getAttributeValue,
         "Return the value of the named attribute")

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
