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
    m_peaks.setParameter(i, value, explicitlySet);
  }
}

  /// Set i-th parameter description
void CrystalFieldSpectrum::setParameterDescription(size_t i,
  const std::string &description) {
  if (i < m_nOwnParams) {
    m_crystalField.setParameterDescription(i, description);
  } else {
    m_peaks.setParameterDescription(i, description);
  }
}

  /// Get i-th parameter
double CrystalFieldSpectrum::getParameter(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getParameter(i)
                          : m_peaks.getParameter(i);
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
  return m_crystalField.nParams() + m_peaks.nParams();
}

  /// Returns the index of parameter name
size_t CrystalFieldSpectrum::parameterIndex(const std::string &name) const {
}

  /// Returns the name of parameter i
std::string CrystalFieldSpectrum::parameterName(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterName(i) : m_peaks.parameterName(i);
}

  /// Returns the description of parameter i
std::string CrystalFieldSpectrum::parameterDescription(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterDescription(i) : m_peaks.parameterDescription(i);
}

  /// Checks if a parameter has been set explicitly
bool CrystalFieldSpectrum::isExplicitlySet(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isExplicitlySet(i) : m_peaks.isExplicitlySet(i);
}

  /// Get the fitting error for a parameter
double CrystalFieldSpectrum::getError(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getError(i) : m_peaks.getError(i);
}

  /// Set the fitting error for a parameter
void CrystalFieldSpectrum::setError(size_t i, double err) {
  if (i < m_nOwnParams) {
    m_crystalField.setError(i, err);
  } else {
    m_peaks.setError(i, err);
  }
}

  /// Check if a declared parameter i is fixed
bool CrystalFieldSpectrum::isFixed(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isFixed(i) : m_peaks.isFixed(i);}

  /// Removes a declared parameter i from the list of active
void CrystalFieldSpectrum::fix(size_t i) {
  if (i < m_nOwnParams) {
    m_crystalField.fix(i);
  } else {
    m_peaks.fix(i);
  }
}

  /// Restores a declared parameter i to the active status
void CrystalFieldSpectrum::unfix(size_t i) {
  if (i < m_nOwnParams) {
    m_crystalField.unfix(i);
  } else {
    m_peaks.unfix(i);
  }
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



} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
