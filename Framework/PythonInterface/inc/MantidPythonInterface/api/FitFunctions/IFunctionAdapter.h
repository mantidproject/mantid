// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_IFUNCTIONADAPTER_H_
#define MANTID_PYTHONINTERFACE_IFUNCTIONADAPTER_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidAPI/IFunction.h"

#include <boost/python/list.hpp>
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
/**
 * Provides a layer to hook into the protected functions
 * of IFunction
 */
class IFunctionAdapter : virtual public API::IFunction {
public:
  /// A constructor that looks like a Python __init__ method
  IFunctionAdapter(PyObject *self, std::string functionMethod,
                   std::string derivMethod);

  /// The PyObject must be supplied to construct the object
  IFunctionAdapter(const IFunctionAdapter &) = delete;

  /// Disable assignment operator
  IFunctionAdapter &operator=(const IFunctionAdapter &) = delete;

  /// Returns the name of the function
  std::string name() const override;
  /// Specify a category for the function
  const std::string category() const override;
  /// Declare all attributes & parameters
  void init() override;

  /// Declare an attribute with an initial value
  void declareAttribute(const std::string &name,
                        const boost::python::object &defaultValue);
  /// Get a named attribute value
  static PyObject *getAttributeValue(IFunction &self, const std::string &name);
  /// Returns the attribute's value as a Python object
  static PyObject *getAttributeValue(IFunction &self,
                                     const API::IFunction::Attribute &attr);
  /// Set the attribute's value
  static void setAttributePythonValue(IFunction &self, const std::string &name,
                                      const boost::python::object &value);
  /// Called by the framework when an attribute has been set
  void setAttribute(const std::string &attName,
                    const API::IFunction::Attribute &attr) override;
  /// Split this function (if needed) into a list of independent functions
  static boost::python::list createPythonEquivalentFunctions(IFunction &self);

  // Each overload of declareParameter requires a different name as we
  // can't use a function pointer with a virtual base class

  /**
   * Declare a named parameter with initial value & description
   * @param name :: The name of the parameter
   * @param initValue :: The initial value
   * @param description :: A short description of the parameter
   */
  inline void declareFitParameter(const std::string &name, double initValue,
                                  const std::string &description) {
    this->declareParameter(name, initValue, description);
  }

  /**
   * Declare a named parameter with initial value
   * @param name :: The name of the parameter
   * @param initValue :: The initial value
   */
  inline void declareFitParameterNoDescr(const std::string &name,
                                         double initValue) {
    this->declareFitParameter(name, initValue, "");
  }

  /**
   * Declare a named parameter with initial value = 0.0
   * @param name :: The name of the parameter
   */
  inline void declareFitParameterZeroInit(const std::string &name) {
    this->declareFitParameter(name, 0.0, "");
  }

  ///  Override this method to make fitted parameters different from the
  ///  declared
  double activeParameter(size_t i) const override;
  /// Override this method to make fitted parameters different from the declared
  void setActiveParameter(size_t i, double value) override;

protected:
  /// @returns The PyObject that owns this wrapper, i.e. self
  inline PyObject *getSelf() const { return m_self; }
  /// @returns True if the instance overrides the derivative method
  inline bool derivativeOverridden() const { return m_derivOveridden; }
  /// Evaluate the function by calling the overridden method
  void evaluateFunction(double *out, const double *xValues,
                        const size_t nData) const;
  /// Evaluate the derivative by calling the overridden method
  void evaluateDerivative(API::Jacobian *out, const double *xValues,
                          const size_t nData) const;

private:
  /// The Python portion of the object
  PyObject *m_self;
  /// The name of the method to evaluate the function
  std::string m_functionName;
  /// The name of the method to evaluate the derivative
  std::string m_derivName;
  /// Flag if the derivateive method is overridden (avoids multiple checks)
  bool m_derivOveridden;
};
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_IFUNCTIONADAPTER_H_ */
