#include "MantidCurveFitting/Functions/FunctionGenerator.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

/// Constructor
FunctionGenerator::FunctionGenerator(API::IFunction_sptr source)
    : m_source(source), m_nOwnParams(source->nParams()), m_dirty(true) {
  if (!m_source) {
    throw std::logic_error(
        "FunctionGenerator initialised with null source function.");
  }
  declareAttribute("NumDeriv", Attribute(false));
}

void FunctionGenerator::init() {}

/// Set i-th parameter
void FunctionGenerator::setParameter(size_t i, const double &value,
                                     bool explicitlySet) {
  if (i < m_nOwnParams) {
    m_source->setParameter(i, value, explicitlySet);
    m_dirty = true;
  } else {
    checkTargetFunction();
    m_target->setParameter(i - m_nOwnParams, value, explicitlySet);
  }
}

/// Set i-th parameter description
void FunctionGenerator::setParameterDescription(
    size_t i, const std::string &description) {
  if (i < m_nOwnParams) {
    m_source->setParameterDescription(i, description);
  } else {
    checkTargetFunction();
    m_target->setParameterDescription(i - m_nOwnParams, description);
  }
}

/// Get i-th parameter
double FunctionGenerator::getParameter(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->getParameter(i)
                          : m_target->getParameter(i - m_nOwnParams);
}

/// Set parameter by name.
void FunctionGenerator::setParameter(const std::string &name,
                                     const double &value, bool explicitlySet) {
  auto i = parameterIndex(name);
  setParameter(i, value, explicitlySet);
}

/// Set description of parameter by name.
void FunctionGenerator::setParameterDescription(
    const std::string &name, const std::string &description) {
  auto i = parameterIndex(name);
  setParameterDescription(i, description);
}

/// Get parameter by name.
double FunctionGenerator::getParameter(const std::string &name) const {
  auto i = parameterIndex(name);
  return getParameter(i);
}

/// Total number of parameters
size_t FunctionGenerator::nParams() const {
  checkTargetFunction();
  return m_source->nParams() + m_target->nParams();
}

/// Returns the index of parameter name
size_t FunctionGenerator::parameterIndex(const std::string &name) const {
  if (isSourceName(name)) {
    return m_source->parameterIndex(name);
  } else {
    checkTargetFunction();
    return m_target->parameterIndex(name) + m_nOwnParams;
  }
}

/// Returns the name of parameter i
std::string FunctionGenerator::parameterName(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->parameterName(i)
                          : m_target->parameterName(i - m_nOwnParams);
}

/// Returns the description of parameter i
std::string FunctionGenerator::parameterDescription(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->parameterDescription(i)
                          : m_target->parameterDescription(i - m_nOwnParams);
}

/// Checks if a parameter has been set explicitly
bool FunctionGenerator::isExplicitlySet(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->isExplicitlySet(i)
                          : m_target->isExplicitlySet(i - m_nOwnParams);
}

/// Get the fitting error for a parameter
double FunctionGenerator::getError(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->getError(i)
                          : m_target->getError(i - m_nOwnParams);
}

/// Set the fitting error for a parameter
void FunctionGenerator::setError(size_t i, double err) {
  if (i < m_nOwnParams) {
    m_source->setError(i, err);
  } else {
    checkTargetFunction();
    m_target->setError(i - m_nOwnParams, err);
  }
}

/// Check if a declared parameter i is fixed
bool FunctionGenerator::isFixed(size_t i) const {
  checkTargetFunction();
  return i < m_nOwnParams ? m_source->isFixed(i)
                          : m_target->isFixed(i - m_nOwnParams);
}

/// Removes a declared parameter i from the list of active
void FunctionGenerator::fix(size_t i) {
  if (i < m_nOwnParams) {
    m_source->fix(i);
  } else {
    checkTargetFunction();
    m_target->fix(i - m_nOwnParams);
  }
}

/// Restores a declared parameter i to the active status
void FunctionGenerator::unfix(size_t i) {
  if (i < m_nOwnParams) {
    m_source->unfix(i);
  } else {
    checkTargetFunction();
    m_target->unfix(i - m_nOwnParams);
  }
}

/// Return parameter index from a parameter reference.
size_t
FunctionGenerator::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getFunction() == this) {
    auto index = ref.getIndex();
    auto np = nParams();
    if (index < np) {
      return index;
    }
    return np;
  }
  checkTargetFunction();
  return m_target->getParameterIndex(ref) + m_nOwnParams;
}

/// Tie a parameter to other parameters (or a constant)
API::ParameterTie *FunctionGenerator::tie(const std::string &parName,
                                          const std::string &expr,
                                          bool isDefault) {
  if (isSourceName(parName)) {
    return m_source->tie(parName, expr, isDefault);
  } else {
    checkTargetFunction();
    return m_target->tie(parName, expr, isDefault);
  }
}

/// Apply the ties
void FunctionGenerator::applyTies() {
  m_source->applyTies();
  updateTargetFunction();
  if (m_target) {
    m_target->applyTies();
  }
}

/// Remove all ties
void FunctionGenerator::clearTies() {
  m_source->clearTies();
  if (m_target) {
    m_target->clearTies();
  }
}

/// Removes i-th parameter's tie
bool FunctionGenerator::removeTie(size_t i) {
  if (i < m_nOwnParams) {
    return m_source->removeTie(i);
  } else {
    checkTargetFunction();
    return m_target->removeTie(i - m_nOwnParams);
  }
}

/// Get the tie of i-th parameter
ParameterTie *FunctionGenerator::getTie(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->getTie(i);
  } else {
    checkTargetFunction();
    return m_target->getTie(i - m_nOwnParams);
  }
}

/// Add a constraint to function
void FunctionGenerator::addConstraint(API::IConstraint *ic) {
  auto i = ic->getIndex();
  if (i < m_nOwnParams) {
    ic->reset(m_source.get(), i);
    m_source->addConstraint(ic);
  } else {
    checkTargetFunction();
    ic->reset(m_target.get(), i - m_nOwnParams);
    m_target->addConstraint(ic);
  }
}

/// Get constraint of i-th parameter
IConstraint *FunctionGenerator::getConstraint(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->getConstraint(i);
  } else {
    checkTargetFunction();
    return m_target->getConstraint(i - m_nOwnParams);
  }
}

/// Remove a constraint
void FunctionGenerator::removeConstraint(const std::string &parName) {
  if (isSourceName(parName)) {
    m_source->removeConstraint(parName);
  } else {
    checkTargetFunction();
    m_target->removeConstraint(parName);
  }
}

/// Set up the function for a fit.
void FunctionGenerator::setUpForFit() { updateTargetFunction(); }

/// Declare a new parameter
void FunctionGenerator::declareParameter(const std::string &, double,
                                         const std::string &) {
  throw Kernel::Exception::NotImplementedError(
      "FunctionGenerator cannot not have its own parameters.");
}

/// Add a new tie. Derived classes must provide storage for ties
void FunctionGenerator::addTie(API::ParameterTie *tie) {
  size_t i = getParameterIndex(*tie);
  if (i < m_nOwnParams) {
    m_source->addTie(tie);
  } else {
    checkTargetFunction();
    tie->reset(m_target.get(), tie->getIndex() - m_nOwnParams,
               tie->isDefault());
    m_target->addTie(tie);
  }
}

/// Returns the number of attributes associated with the function
size_t FunctionGenerator::nAttributes() const {
  checkTargetFunction();
  return IFunction::nAttributes() + m_source->nAttributes() +
         m_target->nAttributes();
}

/// Returns a list of attribute names
std::vector<std::string> FunctionGenerator::getAttributeNames() const {
  checkTargetFunction();
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  auto cfNames = m_source->getAttributeNames();
  auto spNames = m_target->getAttributeNames();
  attNames.insert(attNames.end(), cfNames.begin(), cfNames.end());
  attNames.insert(attNames.end(), spNames.begin(), spNames.end());
  return attNames;
}

/// Return a value of attribute attName
API::IFunction::Attribute
FunctionGenerator::getAttribute(const std::string &attName) const {
  if (IFunction::hasAttribute(attName)) {
    return IFunction::getAttribute(attName);
  } else if (isSourceName(attName)) {
    return m_source->getAttribute(attName);
  } else {
    checkTargetFunction();
    return m_target->getAttribute(attName);
  }
}

/// Set a value to attribute attName
void FunctionGenerator::setAttribute(const std::string &attName,
                                     const IFunction::Attribute &att) {
  if (IFunction::hasAttribute(attName)) {
    IFunction::setAttribute(attName, att);
    m_dirty = true;
    m_target.reset();
  } else if (isSourceName(attName)) {
    m_source->setAttribute(attName, att);
    m_dirty = true;
  } else {
    checkTargetFunction();
    m_target->setAttribute(attName, att);
  }
}

/// Check if attribute attName exists
bool FunctionGenerator::hasAttribute(const std::string &attName) const {
  if (IFunction::hasAttribute(attName)) {
    return true;
  }
  if (isSourceName(attName)) {
    return m_source->hasAttribute(attName);
  } else {
    checkTargetFunction();
    return m_target->hasAttribute(attName);
  }
}

// Evaluates the function
void FunctionGenerator::function(const API::FunctionDomain &domain,
                                 API::FunctionValues &values) const {
  updateTargetFunction();
  if (!m_target) {
    throw std::logic_error(
        "FunctionGenerator failed to generate target function.");
  }
  m_target->function(domain, values);
}

/// Test if a name (parameter's or attribute's) belongs to m_source
/// @param aName :: A name to test.
bool FunctionGenerator::isSourceName(const std::string &aName) const {
  if (aName.empty()) {
    throw std::invalid_argument(
        "Parameter or attribute name cannot be empty string.");
  }
  return (aName.front() != 'f' || aName.find('.') == std::string::npos);
}

/// Update spectrum function if necessary.
void FunctionGenerator::checkTargetFunction() const {
  if (m_dirty) {
    updateTargetFunction();
  }
  if (!m_target) {
    throw std::logic_error(
        "FunctionGenerator failed to generate target function.");
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
