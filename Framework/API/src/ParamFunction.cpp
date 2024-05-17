// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"

#include <cmath>
#include <limits>
#include <sstream>

namespace Mantid::API {
namespace {
Kernel::Logger g_log("ParamFunction");
}

/** Sets a new value to the i-th parameter.
 *  @param i :: The parameter index
 *  @param value :: The new value
 *  @param explicitlySet :: A boolean falgging the parameter as explicitly set
 * (by user)
 */
void ParamFunction::setParameter(size_t i, const double &value, bool explicitlySet) {
  if (std::isnan(value)) {
    // Check for NaN or -NaN
    std::stringstream errmsg;
    errmsg << "Trying to set a NaN value (" << value << ") to parameter " << this->parameterName(i);
    throw std::invalid_argument(errmsg.str());
  } else if (std::isinf(value)) {
    // Infinity value
    std::stringstream errmsg;
    errmsg << "Trying to set an infinity value (" << value << ") to parameter " << this->parameterName(i);
    throw std::invalid_argument(errmsg.str());
  }

  checkParameterIndex(i);
  if (explicitlySet && value != m_parameters[i]) {
    m_explicitlySet[i] = true;
  }
  m_parameters[i] = value;
}

/** Sets a new parameter description to the i-th parameter.
 *  @param i :: The parameter index
 *  @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(size_t i, const std::string &description) {
  checkParameterIndex(i);
  m_parameterDescriptions[i] = description;
}

/** Get the i-th parameter.
 *  @param i :: The parameter index
 *  @return the value of the requested parameter
 */
double ParamFunction::getParameter(size_t i) const {
  checkParameterIndex(i);
  return m_parameters[i];
}

/**
 * Sets a new value to a parameter by name.
 * @param name :: The name of the parameter.
 * @param value :: The new value
 * @param explicitlySet :: A boolean flagging the parameter as explicitly set
 * (by user)
 */
void ParamFunction::setParameter(const std::string &name, const double &value, bool explicitlySet) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to set value to non-exist parameter (" << name << ") "
        << "of function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parName : m_parameterNames) {
      msg << parName << ", ";
    }
    throw std::invalid_argument(msg.str());
  }
  setParameter(static_cast<int>(it - m_parameterNames.begin()), value, explicitlySet);
}

/**
 * Sets a new description to a parameter by name.
 * @param name :: The name of the parameter.
 * @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(const std::string &name, const std::string &description) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to set description to non-exist parameter (" << name << "). ";
    msg << "\nAllowed parameters: ";
    for (const auto &parName : m_parameterNames)
      msg << parName << ", ";
    throw std::invalid_argument(msg.str());
  }
  setParameterDescription(static_cast<int>(it - m_parameterNames.begin()), description);
}

/**
 * Parameters by name.
 * @param name :: The name of the parameter.
 * @return the value of the named parameter
 */
double ParamFunction::getParameter(const std::string &name) const {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to get value of non-existing parameter (" << name << ") "
        << "to function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parName : m_parameterNames)
      msg << parName << ", ";
    throw std::invalid_argument(msg.str());
  }

  double parvalue = m_parameters[it - m_parameterNames.cbegin()];

  if (!std::isfinite(parvalue)) {
    g_log.warning() << "Parameter " << name << " has a NaN or infinity value " << '\n';
  }

  return parvalue;
}

/**
 * Check if function has a parameter with a particular name.
 * @param name :: A name of a parameter.
 * @return True if the parameter exists.
 */
bool ParamFunction::hasParameter(const std::string &name) const {
  return std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name) != m_parameterNames.cend();
}

/**
 * Returns the index of the parameter named name.
 * @param name :: The name of the parameter.
 * @return the index of the named parameter
 */
size_t ParamFunction::parameterIndex(const std::string &name) const {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction " << this->name() << " does not have parameter (" << name << ").";
    throw std::invalid_argument(msg.str());
  }
  return std::distance(m_parameterNames.cbegin(), it);
}

/** Returns the name of parameter i
 * @param i :: The index of a parameter
 * @return the name of the parameter at the requested index
 */
std::string ParamFunction::parameterName(size_t i) const {
  checkParameterIndex(i);
  return m_parameterNames[i];
}

/** Returns the description of parameter i
 * @param i :: The index of a parameter
 * @return the description of the parameter at the requested index
 */
std::string ParamFunction::parameterDescription(size_t i) const {
  checkParameterIndex(i);
  return m_parameterDescriptions[i];
}

/**
 * Get the fitting error for a parameter.
 * @param i :: The index of a parameter
 * @return :: the error
 */
double ParamFunction::getError(size_t i) const {
  checkParameterIndex(i);
  return m_errors[i];
}

/**
 * Get the fitting error for a parameter by name.
 * @param name :: The name of a parameter
 * @return :: the error
 */
double ParamFunction::getError(const std::string &name) const {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to get error of non-existing parameter (" << name << ") "
        << "to function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parName : m_parameterNames)
      msg << parName << ", ";
    throw std::invalid_argument(msg.str());
  }
  return m_errors[static_cast<int>(it - m_parameterNames.begin())];
}

/**
 * Set the fitting error for a parameter.
 * @param i :: The index of a parameter
 * @param err :: The error value to set
 */
void ParamFunction::setError(size_t i, double err) {
  checkParameterIndex(i);
  m_errors[i] = err;
}

/**
 * Get the fitting error for a parameter by name.
 * @param name :: The name of a parameter
 * @param err :: The error value to set
 */
void ParamFunction::setError(const std::string &name, double err) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to set error of non-existing parameter (" << name << ") "
        << "to function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parName : m_parameterNames)
      msg << parName << ", ";
    throw std::invalid_argument(msg.str());
  }
  m_errors[static_cast<int>(it - m_parameterNames.begin())] = err;
}

/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name :: The parameter name.
 * @param initValue :: The initial value for the parameter
 * @param description :: The description for the parameter
 */
void ParamFunction::declareParameter(const std::string &name, double initValue, const std::string &description) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it != m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction parameter (" << name << ") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_parameterStatus.emplace_back(Active);
  m_parameterNames.emplace_back(name);
  m_parameterDescriptions.emplace_back(description);
  m_parameters.emplace_back(initValue);
  m_errors.emplace_back(0.0);
  m_explicitlySet.emplace_back(false);
}

/// Nonvirtual member which removes all declared parameters
void ParamFunction::clearAllParameters() {
  clearTies();
  clearConstraints();
  m_parameters.clear();
  m_parameterNames.clear();
  m_parameterDescriptions.clear();
  m_parameterStatus.clear();
}

/// Change status of parameter
/// @param i :: Index of a parameter.
/// @param status :: New parameter status.
void ParamFunction::setParameterStatus(size_t i, ParameterStatus status) {
  checkParameterIndex(i);
  m_parameterStatus[i] = status;
}

/// Get status of parameter
/// @param i :: Index of a parameter.
/// @return Parameter status.
IFunction::ParameterStatus ParamFunction::getParameterStatus(size_t i) const {
  checkParameterIndex(i);
  return m_parameterStatus[i];
}

/// Get the address of the parameter
/// @param i :: the index of the parameter required
/// @returns the address of the parameter
double *ParamFunction::getParameterAddress(size_t i) {
  checkParameterIndex(i);
  return &m_parameters[i];
}

/// Checks if a parameter has been set explicitly
bool ParamFunction::isExplicitlySet(size_t i) const {
  checkParameterIndex(i);
  return m_explicitlySet[i];
}

/**
 * Returns the index of parameter if the ref points to this ParamFunction
 * @param ref :: A reference to a parameter
 * @return Parameter index or number of nParams() if parameter not found
 */
size_t ParamFunction::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getLocalFunction() == this && ref.getLocalIndex() < nParams()) {
    return ref.getLocalIndex();
  }
  return nParams();
}

} // namespace Mantid::API
