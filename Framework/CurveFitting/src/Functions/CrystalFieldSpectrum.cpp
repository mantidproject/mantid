//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <algorithm>
#include <functional>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>

#include <fstream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(CrystalFieldSpectrum)

/// Constructor
CrystalFieldSpectrum::CrystalFieldSpectrum(): m_nOwnParams(m_crystalField.nParams()) {
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  setAttributeValue("NumDeriv", true);
}

void CrystalFieldSpectrum::init() {}

  /// Set i-th parameter
void CrystalFieldSpectrum::setParameter(size_t i, const double &value,
  bool explicitlySet) {
  if (i < m_nOwnParams) {
    m_crystalField.setParameter(i, value, explicitlySet);
  } else {
    m_spectrum.setParameter(i - m_nOwnParams, value, explicitlySet);
  }
}

  /// Set i-th parameter description
void CrystalFieldSpectrum::setParameterDescription(size_t i,
  const std::string &description) {
  if (i < m_nOwnParams) {
    m_crystalField.setParameterDescription(i, description);
  } else {
    m_spectrum.setParameterDescription(i - m_nOwnParams, description);
  }
}

  /// Get i-th parameter
double CrystalFieldSpectrum::getParameter(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getParameter(i)
                          : m_spectrum.getParameter(i - m_nOwnParams);
}

/// Set parameter by name.
void CrystalFieldSpectrum::setParameter(const std::string &name, const double &value,
  bool explicitlySet) {
  auto i = parameterIndex(name);
  setParameter(i, value, explicitlySet);
}

  /// Set description of parameter by name.
void CrystalFieldSpectrum::setParameterDescription(const std::string &name,
  const std::string &description) {
  auto i = parameterIndex(name);
  setParameterDescription(i, description);
}

  /// Get parameter by name.
double CrystalFieldSpectrum::getParameter(const std::string &name) const {
  auto i = parameterIndex(name);
  return getParameter(i);
}

  /// Total number of parameters
size_t CrystalFieldSpectrum::nParams() const {
  return m_crystalField.nParams() + m_spectrum.nParams();
}

  /// Returns the index of parameter name
size_t CrystalFieldSpectrum::parameterIndex(const std::string &name) const {
  if (name.empty()) {
    throw std::invalid_argument("Parameter name cannot be empty string.");
  }
  if (name.front() != 'f' || name.find('.') == std::string::npos) {
    return m_crystalField.parameterIndex(name);
  } else {
    return m_spectrum.parameterIndex(name);
  }
}

  /// Returns the name of parameter i
std::string CrystalFieldSpectrum::parameterName(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterName(i) : m_spectrum.parameterName(i - m_nOwnParams);
}

  /// Returns the description of parameter i
std::string CrystalFieldSpectrum::parameterDescription(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterDescription(i) : m_spectrum.parameterDescription(i - m_nOwnParams);
}

  /// Checks if a parameter has been set explicitly
bool CrystalFieldSpectrum::isExplicitlySet(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isExplicitlySet(i) : m_spectrum.isExplicitlySet(i - m_nOwnParams);
}

  /// Get the fitting error for a parameter
double CrystalFieldSpectrum::getError(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getError(i) : m_spectrum.getError(i - m_nOwnParams);
}

  /// Set the fitting error for a parameter
void CrystalFieldSpectrum::setError(size_t i, double err) {
  if (i < m_nOwnParams) {
    m_crystalField.setError(i, err);
  } else {
    m_spectrum.setError(i - m_nOwnParams, err);
  }
}

  /// Check if a declared parameter i is fixed
bool CrystalFieldSpectrum::isFixed(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isFixed(i) : m_spectrum.isFixed(i - m_nOwnParams);}

  /// Removes a declared parameter i from the list of active
void CrystalFieldSpectrum::fix(size_t i) {
  if (i < m_nOwnParams) {
    m_crystalField.fix(i);
  } else {
    m_spectrum.fix(i - m_nOwnParams);
  }
}

  /// Restores a declared parameter i to the active status
void CrystalFieldSpectrum::unfix(size_t i) {
  if (i < m_nOwnParams) {
    m_crystalField.unfix(i);
  } else {
    m_spectrum.unfix(i - m_nOwnParams);
  }
}

/// Return parameter index from a parameter reference.
size_t CrystalFieldSpectrum::getParameterIndex(const ParameterReference &ref) const {
  auto index = m_crystalField.getParameterIndex(ref);
  if (index < m_nOwnParams) {
    return index;
  }
  return m_spectrum.getParameterIndex(ref);
}

/// Apply the ties
void CrystalFieldSpectrum::applyTies() {
  m_crystalField.applyTies();
  m_spectrum.applyTies();
}

/// Remove all ties
void CrystalFieldSpectrum::clearTies() {
  m_crystalField.clearTies();
  m_spectrum.clearTies();
}

/// Removes i-th parameter's tie
bool CrystalFieldSpectrum::removeTie(size_t i) {
  if (i < m_nOwnParams) {
    return m_crystalField.removeTie(i);
  } else {
    return m_spectrum.removeTie(i - m_nOwnParams);
  }
}

/// Get the tie of i-th parameter
ParameterTie *CrystalFieldSpectrum::getTie(size_t i) const {
  if (i < m_nOwnParams) {
    return m_crystalField.getTie(i);
  } else {
    return m_spectrum.getTie(i - m_nOwnParams);
  }
}

/// Add a constraint to function
void CrystalFieldSpectrum::addConstraint(IConstraint *ic) {
}

/// Get constraint of i-th parameter
IConstraint *CrystalFieldSpectrum::getConstraint(size_t i) const {
  if (i < m_nOwnParams) {
    return m_crystalField.getConstraint(i);
  } else {
    return m_spectrum.getConstraint(i - m_nOwnParams);
  }
}

/// Remove a constraint
void CrystalFieldSpectrum::removeConstraint(const std::string &parName) {
}

/// Set up the function for a fit.
void CrystalFieldSpectrum::setUpForFit() {
}

/// Declare a new parameter
void CrystalFieldSpectrum::declareParameter(const std::string &, double,
                              const std::string &) {
  throw Kernel::Exception::NotImplementedError(
      "CrystalFieldSpectrum cannot not have its own parameters.");
}

/// Add a new tie. Derived classes must provide storage for ties
void CrystalFieldSpectrum::addTie(ParameterTie *tie) {
}

void CrystalFieldSpectrum::setAttribute(const std::string &attName,
                               const IFunction::Attribute &att) {
  if (attName == "PeakShape") {
  }
  //CompositeFunction::setAttribute(attName, att);
}

// Evaluates the function
void CrystalFieldSpectrum::function1D(double *out, const double *xValues,
                          const size_t nData) const {
}

/// Uses m_crystalField to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldSpectrum::buildSpectrumFunction() {
  FunctionDomainGeneral domain;
  FunctionValues values;
  m_crystalField.functionGeneral(domain, values);
}


} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
