//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"

#include <cmath>
#include <limits>
#include <sstream>

namespace Mantid {
namespace API {
namespace {
Kernel::Logger g_log("ParamFunction");
}

/// Destructor
ParamFunction::~ParamFunction() {
  m_ties.clear();
  m_constraints.clear();
}

/** Sets a new value to the i-th parameter.
 *  @param i :: The parameter index
 *  @param value :: The new value
 *  @param explicitlySet :: A boolean falgging the parameter as explicitly set
 * (by user)
 */
void ParamFunction::setParameter(size_t i, const double &value,
                                 bool explicitlySet) {
  if (std::isnan(value)) {
    // Check for NaN or -NaN
    std::stringstream errmsg;
    errmsg << "Trying to set a NaN value (" << value << ") to parameter "
           << this->parameterName(i);
    g_log.warning(errmsg.str());
  } else if (std::isinf(value)) {
    // Infinity value
    std::stringstream errmsg;
    errmsg << "Trying to set an infinity value (" << value << ") to parameter "
           << this->parameterName(i);
    g_log.warning(errmsg.str());
  }

  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  if (explicitlySet && value != m_parameters[i]) {
    m_explicitlySet[i] = true;
  }
  m_parameters[i] = value;
}

/** Sets a new parameter description to the i-th parameter.
 *  @param i :: The parameter index
 *  @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(size_t i,
                                            const std::string &description) {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  m_parameterDescriptions[i] = description;
}

/** Get the i-th parameter.
 *  @param i :: The parameter index
 *  @return the value of the requested parameter
 */
double ParamFunction::getParameter(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameters[i];
}

/**
 * Sets a new value to a parameter by name.
 * @param name :: The name of the parameter.
 * @param value :: The new value
 * @param explicitlySet :: A boolean flagging the parameter as explicitly set
 * (by user)
 */
void ParamFunction::setParameter(const std::string &name, const double &value,
                                 bool explicitlySet) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to set value to non-exist parameter (" << name
        << ") "
        << "of function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parameterName : m_parameterNames) {
      msg << parameterName << ", ";
    }
    throw std::invalid_argument(msg.str());
  }
  setParameter(static_cast<int>(it - m_parameterNames.begin()), value,
               explicitlySet);
}

/**
 * Sets a new description to a parameter by name.
 * @param name :: The name of the parameter.
 * @param description :: New parameter description
 */
void ParamFunction::setParameterDescription(const std::string &name,
                                            const std::string &description) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it == m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction tries to set description to non-exist parameter ("
        << name << "). ";
    msg << "\nAllowed parameters: ";
    for (const auto &parameterName : m_parameterNames)
      msg << parameterName << ", ";
    throw std::invalid_argument(msg.str());
  }
  setParameterDescription(static_cast<int>(it - m_parameterNames.begin()),
                          description);
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
    msg << "ParamFunction tries to get value of non-existing parameter ("
        << name << ") "
        << "to function " << this->name();
    msg << "\nAllowed parameters: ";
    for (const auto &parameterName : m_parameterNames)
      msg << parameterName << ", ";
    throw std::invalid_argument(msg.str());
  }

  double parvalue = m_parameters[it - m_parameterNames.cbegin()];

  if (!std::isfinite(parvalue)) {
    g_log.warning() << "Parameter " << name << " has a NaN or infinity value "
                    << '\n';
  }

  return parvalue;
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
    msg << "ParamFunction " << this->name() << " does not have parameter ("
        << name << ").";
    throw std::invalid_argument(msg.str());
  }
  return std::distance(m_parameterNames.cbegin(), it);
}

/** Returns the name of parameter i
 * @param i :: The index of a parameter
 * @return the name of the parameter at the requested index
 */
std::string ParamFunction::parameterName(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameterNames[i];
}

/** Returns the description of parameter i
 * @param i :: The index of a parameter
 * @return the description of the parameter at the requested index
 */
std::string ParamFunction::parameterDescription(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_parameterDescriptions[i];
}

/**
 * Get the fitting error for a parameter
 * @param i :: The index of a parameter
 * @return :: the error
 */
double ParamFunction::getError(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_errors[i];
}

/**
 * Set the fitting error for a parameter
 * @param i :: The index of a parameter
 * @param err :: The error value to set
 */
void ParamFunction::setError(size_t i, double err) {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  m_errors[i] = err;
}

/**
 * Declare a new parameter. To used in the implementation'c constructor.
 * @param name :: The parameter name.
 * @param initValue :: The initial value for the parameter
 * @param description :: The description for the parameter
 */
void ParamFunction::declareParameter(const std::string &name, double initValue,
                                     const std::string &description) {
  auto it = std::find(m_parameterNames.cbegin(), m_parameterNames.cend(), name);
  if (it != m_parameterNames.cend()) {
    std::ostringstream msg;
    msg << "ParamFunction parameter (" << name << ") already exists.";
    throw std::invalid_argument(msg.str());
  }

  m_isFixed.push_back(false);
  m_parameterNames.push_back(name);
  m_parameterDescriptions.push_back(description);
  m_parameters.push_back(initValue);
  m_errors.push_back(0.0);
  m_explicitlySet.push_back(false);
}

/**
 * query if the parameter is fixed
 * @param i :: The index of a declared parameter
 * @return true if parameter i is active
 */
bool ParamFunction::isFixed(size_t i) const {
  if (i >= nParams())
    throw std::out_of_range("ParamFunction parameter index out of range.");
  return m_isFixed[i];
}

/** This method doesn't create a tie
 * @param i :: A declared parameter index to be fixed
 */
void ParamFunction::fix(size_t i) {
  if (isFixed(i))
    return;
  m_isFixed[i] = true;
}

/** Makes a parameter active again. It doesn't change the parameter's tie.
 * @param i :: A declared parameter index to be restored to active
 */
void ParamFunction::unfix(size_t i) {
  if (!isFixed(i))
    return;
  m_isFixed[i] = false;
}

/**
 * Attaches a tie to this ParamFunction. The attached tie is owned by the
 * ParamFunction.
 * @param tie :: A pointer to a new tie
 */
void ParamFunction::addTie(std::unique_ptr<ParameterTie> tie) {
  size_t iPar = tie->getIndex();
  bool found = false;
  for (auto &m_tie : m_ties) {
    if (m_tie->getIndex() == iPar) {
      found = true;
      m_tie = std::move(tie);
      break;
    }
  }
  if (!found) {
    m_ties.push_back(std::move(tie));
  }
}

/**
 * Apply the ties.
 */
void ParamFunction::applyTies() {
  for (auto &m_tie : m_ties) {
    m_tie->eval();
  }
}

/**
 * Used to find ParameterTie for a parameter i
 */
class ReferenceEqual {
  /// index to find
  const size_t m_i;

public:
  /// Constructor
  explicit ReferenceEqual(size_t i) : m_i(i) {}
  /// Bracket operator
  /// @param p :: the element you are looking for
  /// @return True if found
  template <class T> bool operator()(const std::unique_ptr<T> &p) {
    return p->getIndex() == m_i;
  }
};

/** Removes i-th parameter's tie if it is tied or does nothing.
 * @param i :: The index of the tied parameter.
 * @return True if successfull
 */
bool ParamFunction::removeTie(size_t i) {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  auto it = std::find_if(m_ties.begin(), m_ties.end(), ReferenceEqual(i));
  if (it != m_ties.end()) {
    m_ties.erase(it);
    unfix(i);
    return true;
  }
  unfix(i);
  return false;
}

/** Get tie of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the tie
 */
ParameterTie *ParamFunction::getTie(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  auto it = std::find_if(m_ties.cbegin(), m_ties.cend(), ReferenceEqual(i));
  if (it != m_ties.cend()) {
    return it->get();
  }
  return nullptr;
}

/** Remove all ties
 */
void ParamFunction::clearTies() {
  for (size_t i = 0; i < nParams(); ++i) {
    unfix(i);
  }
  m_ties.clear();
}

/** Add a constraint
 *  @param ic :: Pointer to a constraint.
 */
void ParamFunction::addConstraint(std::unique_ptr<IConstraint> ic) {
  size_t iPar = ic->getIndex();
  bool found = false;
  for (auto &constraint : m_constraints) {
    if (constraint->getIndex() == iPar) {
      found = true;
      constraint = std::move(ic);
      break;
    }
  }
  if (!found) {
    m_constraints.push_back(std::move(ic));
  }
}

/** Get constraint of parameter number i
 * @param i :: The index of a declared parameter.
 * @return A pointer to the constraint or NULL
 */
IConstraint *ParamFunction::getConstraint(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  auto it = std::find_if(m_constraints.cbegin(), m_constraints.cend(),
                         ReferenceEqual(i));
  if (it != m_constraints.cend()) {
    return it->get();
  }
  return nullptr;
}

/** Remove a constraint
 * @param parName :: The name of a parameter which constarint to remove.
 */
void ParamFunction::removeConstraint(const std::string &parName) {
  size_t iPar = parameterIndex(parName);
  for (auto it = m_constraints.begin(); it != m_constraints.end(); ++it) {
    if (iPar == (**it).getIndex()) {
      m_constraints.erase(it);
      break;
    }
  }
}

void ParamFunction::setUpForFit() {
  for (auto &constraint : m_constraints) {
    constraint->setParamToSatisfyConstraint();
  }
}

/// Nonvirtual member which removes all declared parameters
void ParamFunction::clearAllParameters() {
  m_ties.clear();
  m_constraints.clear();
  m_parameters.clear();
  m_parameterNames.clear();
  m_parameterDescriptions.clear();
  m_isFixed.clear();
}

/// Get the address of the parameter
/// @param i :: the index of the parameter required
/// @returns the address of the parameter
double *ParamFunction::getParameterAddress(size_t i) {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return &m_parameters[i];
}

/// Checks if a parameter has been set explicitly
bool ParamFunction::isExplicitlySet(size_t i) const {
  if (i >= nParams()) {
    throw std::out_of_range("ParamFunction parameter index out of range.");
  }
  return m_explicitlySet[i];
}

/**
 * Returns the index of parameter if the ref points to this ParamFunction
 * @param ref :: A reference to a parameter
 * @return Parameter index or number of nParams() if parameter not found
 */
size_t ParamFunction::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getFunction() == this && ref.getIndex() < nParams()) {
    return ref.getIndex();
  }
  return nParams();
}

} // namespace API
} // namespace Mantid
