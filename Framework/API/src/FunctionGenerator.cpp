// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionGenerator.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/ParameterTie.h"

namespace Mantid {
namespace API {

using namespace Kernel;

/// Constructor
FunctionGenerator::FunctionGenerator(IFunction_sptr source)
    : m_source(source), m_dirty(true) {
  if (source) {
    m_nOwnParams = source->nParams();
  }
  declareAttribute("NumDeriv", Attribute(false));
}

void FunctionGenerator::init() {}

/// Set the source function
/// @param source :: New source function.
void FunctionGenerator::setSource(IFunction_sptr source) const {
  m_source = source;
}

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
  if (i < m_nOwnParams) {
    return m_source->getParameter(i);
  } else {
    checkTargetFunction();
    return m_target->getParameter(i - m_nOwnParams);
  }
}

/// Check if function has a parameter with a particular name.
bool FunctionGenerator::hasParameter(const std::string &name) const {
  if (isSourceName(name)) {
    return m_source->hasParameter(name);
  } else {
    checkTargetFunction();
    return m_target->hasParameter(name);
  }
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
  if (i < m_nOwnParams) {
    return m_source->parameterName(i);
  } else {
    checkTargetFunction();
    return m_target->parameterName(i - m_nOwnParams);
  }
}

/// Returns the description of parameter i
std::string FunctionGenerator::parameterDescription(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->parameterDescription(i);
  } else {
    checkTargetFunction();
    return m_target->parameterDescription(i - m_nOwnParams);
  }
}

/// Checks if a parameter has been set explicitly
bool FunctionGenerator::isExplicitlySet(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->isExplicitlySet(i);
  } else {
    checkTargetFunction();
    return m_target->isExplicitlySet(i - m_nOwnParams);
  }
}

/// Get the fitting error for a parameter
double FunctionGenerator::getError(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->getError(i);
  } else {
    checkTargetFunction();
    return m_target->getError(i - m_nOwnParams);
  }
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

/// Change status of parameter
void FunctionGenerator::setParameterStatus(size_t i,
                                           IFunction::ParameterStatus status) {
  if (i < m_nOwnParams) {
    m_source->setParameterStatus(i, status);
  } else {
    checkTargetFunction();
    m_target->setParameterStatus(i - m_nOwnParams, status);
  }
}

/// Get status of parameter
IFunction::ParameterStatus
FunctionGenerator::getParameterStatus(size_t i) const {
  if (i < m_nOwnParams) {
    return m_source->getParameterStatus(i);
  } else {
    checkTargetFunction();
    return m_target->getParameterStatus(i - m_nOwnParams);
  }
}

/// Return parameter index from a parameter reference.
size_t
FunctionGenerator::getParameterIndex(const ParameterReference &ref) const {
  if (ref.getLocalFunction() == this) {
    auto index = ref.getLocalIndex();
    auto np = nParams();
    if (index < np) {
      return index;
    }
    return np;
  }
  checkTargetFunction();
  return m_target->getParameterIndex(ref) + m_nOwnParams;
}

/// Set up the function for a fit.
void FunctionGenerator::setUpForFit() {
  updateTargetFunction();
  IFunction::setUpForFit();
}

/// Declare a new parameter
void FunctionGenerator::declareParameter(const std::string & /*name*/,
                                         double /*initValue*/,
                                         const std::string & /*description*/) {
  throw Kernel::Exception::NotImplementedError(
      "FunctionGenerator cannot not have its own parameters.");
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
IFunction::Attribute
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
void FunctionGenerator::function(const FunctionDomain &domain,
                                 FunctionValues &values) const {
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

/// Get the tie for i-th parameter
ParameterTie *FunctionGenerator::getTie(size_t i) const {
  auto tie = IFunction::getTie(i);
  if (!tie) {
    return nullptr;
  }
  if (i < m_nOwnParams) {
    tie = m_source->getTie(i);
  } else {
    checkTargetFunction();
    tie = m_target->getTie(i - m_nOwnParams);
  }
  return tie;
}

/// Get the i-th constraint
IConstraint *FunctionGenerator::getConstraint(size_t i) const {
  auto constraint = IFunction::getConstraint(i);
  if (constraint == nullptr) {
    if (i < m_nOwnParams) {
      constraint = m_source->getConstraint(i);
    } else {
      checkTargetFunction();
      constraint = m_target->getConstraint(i - m_nOwnParams);
    }
  }
  return constraint;
}

} // namespace API
} // namespace Mantid
