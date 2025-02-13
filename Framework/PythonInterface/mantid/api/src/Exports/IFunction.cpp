// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/FitFunctions/IFunctionAdapter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/IsNone.h"
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/manage_new_object.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/to_python_value.hpp>

#include <memory>
#include <string>

using Mantid::API::IFunction;
using Mantid::API::IFunction_sptr;
using Mantid::PythonInterface::IFunctionAdapter;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IFunction)

namespace {
///@cond

/**
 * Extracts a pointer to the C++ IFunction from the python object, and then clones the IFunction. The clone is necessary
 * to force a separation from the python object to avoid GIL issues.
 * @param value :: A boost python object that stores the value
 * @returns A clone of the extracted IFunction C++ object.
 */
template <typename T> inline T extractValueAndClone(const boost::python::object &value) {
  return boost::python::extract<T>(value)()->clone();
}

struct FunctionPropertyValueHandler : public PropertyValueHandler {

  /// Type required by TypeRegistry framework
  using HeldType = std::shared_ptr<IFunction>;

  /**
   * Set function to handle Python -> C++ calls and get the correct type
   * @param alg :: A pointer to an IPropertyManager
   * @param name :: The name of the property
   * @param value :: A boost python object that stores the value
   */
  void set(Mantid::Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::object &value) const override {
    alg->setProperty<HeldType>(name, extractValueAndClone<HeldType>(value));
  }

  /**
   * Create a PropertyWithValue from the given python object value
   * @param name :: The name of the property
   * @param defaultValue :: The defaultValue of the property. The object
   * attempts to extract
   * a value of type ValueType from the python object
   * @param validator :: A python object pointing to a validator instance, which
   * can be None.
   * @param direction :: The direction of the property
   * @returns A pointer to a newly constructed property instance
   */
  std::unique_ptr<Mantid::Kernel::Property> create(const std::string &name, const boost::python::object &defaultValue,
                                                   const boost::python::object &validator,
                                                   const unsigned int direction) const override {
    using Mantid::Kernel::IValidator;
    using Mantid::Kernel::PropertyWithValue;
    if (Mantid::PythonInterface::isNone(validator)) {
      return std::make_unique<PropertyWithValue<HeldType>>(name, extractValueAndClone<HeldType>(defaultValue),
                                                           direction);
    }
    const IValidator *propValidator = boost::python::extract<IValidator *>(validator);
    return std::make_unique<PropertyWithValue<HeldType>>(name, extractValueAndClone<HeldType>(defaultValue),
                                                         propValidator->clone(), direction);
  }
};

//------------------------------------------------------------------------------------------------------
/**
 * A Python friendly version that returns the registered functions as a list.
 * @param self :: Enables it to be called as a member function on the
 * FunctionFactory class
 */
PyObject *getCategories(const IFunction &self) {
  const auto categories = self.categories();

  PyObject *registered = PyList_New(0);
  for (const auto &category : categories) {
    PyObject *value = to_python_value<const std::string &>()(category);
    if (PyList_Append(registered, value))
      throw std::runtime_error("Failed to insert value into PyList");
  }

  return registered;
}

// -- Set property overloads --
// setProperty(index,value,explicit)
using setParameterType1 = void (IFunction::*)(size_t, const double &, bool);
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType1_Overloads, setParameter, 2, 3)
// setProperty(name,value,explicit)
using setParameterType2 = void (IFunction::*)(const std::string &, const double &, bool);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setParameterType2_Overloads, setParameter, 2, 3)

// setError(index,value)
using setErrorType1 = void (IFunction::*)(size_t, double);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setErrorType1_Overloads, setError, 2, 2)
// setError(name,value)
using setErrorType2 = void (IFunction::*)(const std::string &, double);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(setErrorType2_Overloads, setError, 2, 2)

// getError(index)
using getErrorType1 = double (IFunction::*)(size_t) const;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getErrorType1_Overloads, getError, 1, 1)
// getError(name)
using getErrorType2 = double (IFunction::*)(const std::string &) const;
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getErrorType2_Overloads, getError, 1, 1)

// declareAttribute(name, defaultValue)
using declareAttributeType1 = void (IFunctionAdapter::*)(const std::string &name,
                                                         const boost::python::object &defaultValue);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declareAttributeType1_Overloads, declareAttribute, 2, 2)
// declareAttribute(name, defaultValue, validator)
using declareAttributeType2 = void (IFunctionAdapter::*)(const std::string &name,
                                                         const boost::python::object &defaultValue,
                                                         const boost::python::object &validator);
// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(declareAttributeType2_Overloads, declareAttribute, 3, 3)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(tie_Overloads, tie, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(addTies_Overloads, addTies, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(addConstraints_Overloads, addConstraints, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(fixParameter_Overloads, fixParameter, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(fix_Overloads, fix, 1, 2)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(fixAll_Overloads, fixAll, 0, 1)
using removeTieByName = void (IFunction::*)(const std::string &);

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
///@endcond

Mantid::API::Jacobian *getFunctionDeriv(IFunction &self, const Mantid::API::FunctionDomain &domain) {
  auto *out = new Mantid::CurveFitting::Jacobian(domain.size(), self.nParams());
  self.functionDeriv(domain, *out);
  return out;
}

void setMatrixWorkspace(IFunction &self, const boost::python::object &workspace, int wi, float startX, float endX) {
  Mantid::API::MatrixWorkspace_sptr matWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::PythonInterface::ExtractSharedPtr<Mantid::API::Workspace>(workspace)());
  self.setMatrixWorkspace(matWS, wi, startX, endX);
}

} // namespace

void export_IFunction() {

  register_ptr_to_python<std::shared_ptr<IFunction>>();

  class_<IFunction, IFunctionAdapter, boost::noncopyable>("IFunction", "Base class for all functions", no_init)
      .def("name", &IFunction::name, arg("self"), "Return the name of the function")

      .def("category", &IFunctionAdapter::category, arg("self"),
           "Return a semi-colon(;) separated string for the categories this "
           "class should belong to. For sub-categories use a \\ separator")

      .def("clone", &IFunction::clone, arg("self"), "Clones the function")

      .def("initialize", &IFunction::initialize, arg("self"), "Declares any parameters and attributes on the function")

      .def("getCategories", &getCategories, arg("self"), "Returns a list of the categories for an algorithm")

      .def("nAttributes", &IFunction::nAttributes, arg("self"),
           "Return the number of attributes (non-fitting arguments)")

      .def("attributeNames", &IFunction::getAttributeNames, arg("self"), "The names of all the attributes")

      .def("hasAttribute", &IFunction::hasAttribute, (arg("self"), arg("name")),
           "Return whether there is an attribute of the given name")

      .def("hasParameter", &IFunction::hasParameter, (arg("self"), arg("name")),
           "Return whether there is an parameter of the given name")

      .def("nParams", &IFunction::nParams, arg("self"), "Return the number of parameters")

      .def("parameterName", &IFunction::parameterName, (arg("self"), arg("i")), "Return the name of the ith parameter")

      .def("paramDescription", &IFunction::parameterDescription, (arg("self"), arg("i")),
           "Return a description of the ith parameter")

      .def("isExplicitlySet", &IFunction::isExplicitlySet, (arg("self"), arg("i")),
           "Return whether the ith parameter needs to be explicitely set")

      .def("getParameterValue", (double(IFunction::*)(size_t) const) & IFunction::getParameter, (arg("self"), arg("i")),
           "Get the value of the ith parameter")

      .def("getParameterValue", (double(IFunction::*)(const std::string &) const) & IFunction::getParameter,
           (arg("self"), arg("name")), "Get the value of the named parameter")

      .def("__getitem__", (double(IFunction::*)(const std::string &) const) & IFunction::getParameter,
           (arg("self"), arg("name")), "Get the value of the named parameter")

      .def("setParameter", (setParameterType1)&IFunction::setParameter,
           setParameterType1_Overloads((arg("self"), arg("i"), arg("value"), arg("explicitlySet")),
                                       "Sets the value of the ith parameter"))

      .def("setParameter", (setParameterType2)&IFunction::setParameter,
           setParameterType2_Overloads((arg("self"), arg("name"), arg("value"), arg("explicitlySet")),
                                       "Sets the value of the named parameter"))

      .def(
          "setError", (setErrorType1)&IFunction::setError,
          setErrorType1_Overloads((args("self"), arg("index"), args("err")), "Sets the error on the indexed parameter"))

      .def("setError", (setErrorType2)&IFunction::setError,
           setErrorType2_Overloads((args("self"), arg("name"), args("err")), "Sets the error on the named parameter"))

      .def("getError", (getErrorType1)&IFunction::getError,
           getErrorType1_Overloads((arg("self"), arg("index")), "Return fitting error of the index parameter"))

      .def("getError", (getErrorType2)&IFunction::getError,
           getErrorType2_Overloads((arg("self"), arg("name")), "Return fitting error of the named parameter"))

      .def("__setitem__", (setParameterType2)&IFunction::setParameter,
           setParameterType2_Overloads((arg("self"), arg("name"), arg("value"), arg("explicitlySet")),
                                       "Sets the value of the named parameter"))

      .def("declareAttribute", (declareAttributeType1)&IFunctionAdapter::declareAttribute,
           declareAttributeType1_Overloads((arg("self"), arg("name"), arg("default_value")),
                                           "Declare an attribute with an initial value"))

      .def("declareAttribute", (declareAttributeType2)&IFunctionAdapter::declareAttribute,
           declareAttributeType2_Overloads((arg("self"), arg("name"), arg("default_value"), arg("validator")),
                                           "Declare an attribute with an initial value, with a validator"))

      .def("getAttributeValue",
           (PyObject * (*)(const IFunction &, const std::string &)) IFunctionAdapter::getAttributeValue,
           (arg("self"), arg("name")), "Return the value of the named attribute")

      .def("setAttributeValue", &IFunctionAdapter::setAttributePythonValue, (arg("self"), arg("name"), arg("value")),
           "Set a value of a named attribute")

      .def("declareParameter", &IFunctionAdapter::declareFitParameter,
           (arg("self"), arg("name"), arg("init_value"), arg("description")),
           "Declare a fitting parameter settings its default value & "
           "description")

      .def("declareParameter", &IFunctionAdapter::declareFitParameterNoDescr,
           (arg("self"), arg("name"), arg("init_value")), "Declare a fitting parameter settings its default value")

      .def("declareParameter", &IFunctionAdapter::declareFitParameterZeroInit, (arg("self"), arg("name")),
           "Declare a fitting parameter settings its default value to 0.0")

      .def("fixParameter", &IFunction::fix,
           fix_Overloads((arg("self"), arg("i"), arg("isDefault")), "Fix the ith parameter"))

      .def("fixParameter", &IFunction::fixParameter,
           fixParameter_Overloads((arg("self"), arg("name"), arg("isDefault")), "Fix the named parameter"))

      .def("freeParameter", &IFunction::unfix, (arg("self"), arg("i")), "Free the ith parameter")

      .def("freeParameter", &IFunction::unfixParameter, (arg("self"), arg("name")), "Free the named parameter")

      .def("isFixed", &IFunction::isFixed, (arg("self"), arg("i")), "Return whether the ith parameter is fixed or tied")

      .def("fixAll", &IFunction::fixAll, fixAll_Overloads((arg("self"), arg("isDefault")), "Fix all parameters"))

      .def("freeAll", &IFunction::unfixAll, (arg("self")), "Free all parameters")

      .def("tie", &IFunction::tie,
           tie_Overloads((arg("self"), arg("name"), arg("expr"), arg("isDefault")),
                         "Tie a named parameter to an expression"))

      .def("addTies", &IFunction::addTies,
           addTies_Overloads((arg("self"), arg("ties"), arg("isDefault")), "Add several ties to an IFunction."))

      .def("removeTie", (bool(IFunction::*)(size_t)) & IFunction::removeTie, (arg("self"), arg("i")),
           "Remove the tie of the ith parameter")

      .def("removeTie", (void(IFunction::*)(const std::string &)) & IFunction::removeTie, (arg("self"), arg("name")),
           "Remove the tie of the named parameter")

      .def("getTies", &IFunction::writeTies, arg("self"), "Returns the list of current ties as a string")

      .def("addConstraints", &IFunction::addConstraints,
           addConstraints_Overloads((arg("self"), arg("constraints"), arg("isDefault")), "Constrain named parameters"))

      .def("removeConstraint", &IFunction::removeConstraint, (arg("self"), arg("name")),
           "Remove the constraint on the named parameter")

      .def("getConstraints", &IFunction::writeConstraints, arg("self"),
           "Returns the list of current constraints as a string")

      .def("setConstraintPenaltyFactor", &IFunction::setConstraintPenaltyFactor,
           (arg("self"), arg("name"), arg("value")), "Set the constraint penalty factor for named parameter")

      .def("getNumberDomains", &IFunction::getNumberDomains, (arg("self")),
           "Get number of domains of a multi-domain function")

      .def("createEquivalentFunctions", &IFunctionAdapter::createPythonEquivalentFunctions, (arg("self")),
           "Split this function (if needed) into a list of "
           "independent functions")

      .def("getFunction", &IFunction::getFunction, (arg("self"), arg("index")),
           "Returns the pointer to i-th child function")

      .def("nDomains", &IFunction::getNumberDomains, arg("self"), "Get the number of domains.")

      .def("functionDeriv", &getFunctionDeriv, (arg("self"), arg("domain")), return_value_policy<manage_new_object>(),
           "Calculate the values of the function for the given domain and returns them")

      .def("setMatrixWorkspace", &setMatrixWorkspace,
           (arg("self"), arg("workspace"), arg("wi"), arg("startX"), arg("endX")),
           "Set matrix workspace to parse Parameters.xml")

      //-- Deprecated functions that have the wrong names --
      .def("categories", &getCategories, arg("self"), "Returns a list of the categories for an algorithm")
      .def("numParams", &IFunction::nParams, arg("self"), "Return the number of parameters")
      .def("getParamName", &IFunction::parameterName, (arg("self"), arg("i")), "Return the name of the ith parameter")
      .def("getParamDescr", &IFunction::parameterDescription, (arg("self"), arg("i")),
           "Return a description of the ith parameter")
      .def("getParamExplicit", &IFunction::isExplicitlySet, (arg("self"), arg("i")),
           "Return whether the ith parameter needs to be explicitely set")
      .def("getParamValue", (double(IFunction::*)(std::size_t) const) & IFunction::getParameter,
           (arg("self"), arg("i")), "Get the value of the ith parameter")
      .def("getParameterIndex", &IFunction::parameterIndex, (arg("self"), arg("name")),
           "Returns the index of the provided parameter.")

      //-- Python special methods --
      .def("__repr__", &IFunction::asString, arg("self"), "Return a string representation of the function");

  TypeRegistry::subscribe<FunctionPropertyValueHandler>();
}
