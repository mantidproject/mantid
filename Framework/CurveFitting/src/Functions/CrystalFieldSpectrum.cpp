//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParameterTie.h"

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
CrystalFieldSpectrum::CrystalFieldSpectrum()
    : m_nOwnParams(m_crystalField.nParams()), m_dirty(true) {
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHM", Attribute(0.0));
}

void CrystalFieldSpectrum::init() {}

/// Set i-th parameter
void CrystalFieldSpectrum::setParameter(size_t i, const double &value,
                                        bool explicitlySet) {
  if (i < m_nOwnParams) {
    m_crystalField.setParameter(i, value, explicitlySet);
    m_dirty = true;
  } else {
    checkSpectrumFunction();
    m_spectrum.setParameter(i - m_nOwnParams, value, explicitlySet);
  }
}

/// Set i-th parameter description
void CrystalFieldSpectrum::setParameterDescription(
    size_t i, const std::string &description) {
  if (i < m_nOwnParams) {
    m_crystalField.setParameterDescription(i, description);
  } else {
    checkSpectrumFunction();
    m_spectrum.setParameterDescription(i - m_nOwnParams, description);
  }
}

/// Get i-th parameter
double CrystalFieldSpectrum::getParameter(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getParameter(i)
                          : m_spectrum.getParameter(i - m_nOwnParams);
}

/// Set parameter by name.
void CrystalFieldSpectrum::setParameter(const std::string &name,
                                        const double &value,
                                        bool explicitlySet) {
  auto i = parameterIndex(name);
  setParameter(i, value, explicitlySet);
}

/// Set description of parameter by name.
void CrystalFieldSpectrum::setParameterDescription(
    const std::string &name, const std::string &description) {
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
  if (isOwnName(name)) {
    return m_crystalField.parameterIndex(name);
  } else {
    checkSpectrumFunction();
    return m_spectrum.parameterIndex(name) + m_nOwnParams;
  }
}

/// Returns the name of parameter i
std::string CrystalFieldSpectrum::parameterName(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterName(i)
                          : m_spectrum.parameterName(i - m_nOwnParams);
}

/// Returns the description of parameter i
std::string CrystalFieldSpectrum::parameterDescription(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.parameterDescription(i)
                          : m_spectrum.parameterDescription(i - m_nOwnParams);
}

/// Checks if a parameter has been set explicitly
bool CrystalFieldSpectrum::isExplicitlySet(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isExplicitlySet(i)
                          : m_spectrum.isExplicitlySet(i - m_nOwnParams);
}

/// Get the fitting error for a parameter
double CrystalFieldSpectrum::getError(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.getError(i)
                          : m_spectrum.getError(i - m_nOwnParams);
}

/// Set the fitting error for a parameter
void CrystalFieldSpectrum::setError(size_t i, double err) {
  if (i < m_nOwnParams) {
    m_crystalField.setError(i, err);
  } else {
    checkSpectrumFunction();
    m_spectrum.setError(i - m_nOwnParams, err);
  }
}

/// Check if a declared parameter i is fixed
bool CrystalFieldSpectrum::isFixed(size_t i) const {
  return i < m_nOwnParams ? m_crystalField.isFixed(i)
                          : m_spectrum.isFixed(i - m_nOwnParams);
}

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
size_t
CrystalFieldSpectrum::getParameterIndex(const ParameterReference &ref) const {
  auto index = m_crystalField.getParameterIndex(ref);
  if (index < m_nOwnParams) {
    return index;
  }
  return m_spectrum.getParameterIndex(ref) + m_nOwnParams;
}

/// Tie a parameter to other parameters (or a constant)
API::ParameterTie *CrystalFieldSpectrum::tie(const std::string &parName,
                                             const std::string &expr,
                                             bool isDefault) {
  if (isOwnName(parName)) {
    return m_crystalField.tie(parName, expr, isDefault);
  } else {
    checkSpectrumFunction();
    return m_spectrum.tie(parName, expr, isDefault);
  }
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
void CrystalFieldSpectrum::addConstraint(API::IConstraint *ic) {
  auto i = ic->getIndex();
  if (i < m_nOwnParams) {
    ic->reset(&m_crystalField, i);
    m_crystalField.addConstraint(ic);
  } else {
    checkSpectrumFunction();
    ic->reset(&m_spectrum, i - m_nOwnParams);
    m_spectrum.addConstraint(ic);
  }
}

/// Get constraint of i-th parameter
IConstraint *CrystalFieldSpectrum::getConstraint(size_t i) const {
  if (i < m_nOwnParams) {
    return m_crystalField.getConstraint(i);
  } else {
    checkSpectrumFunction();
    return m_spectrum.getConstraint(i - m_nOwnParams);
  }
}

/// Remove a constraint
void CrystalFieldSpectrum::removeConstraint(const std::string &parName) {
  if (isOwnName(parName)) {
    m_crystalField.removeConstraint(parName);
  } else {
    checkSpectrumFunction();
    m_spectrum.removeConstraint(parName);
  }
}

/// Set up the function for a fit.
void CrystalFieldSpectrum::setUpForFit() { updateSpectrumFunction(); }

/// Declare a new parameter
void CrystalFieldSpectrum::declareParameter(const std::string &, double,
                                            const std::string &) {
  throw Kernel::Exception::NotImplementedError(
      "CrystalFieldSpectrum cannot not have its own parameters.");
}

/// Add a new tie. Derived classes must provide storage for ties
void CrystalFieldSpectrum::addTie(API::ParameterTie *tie) {
  size_t i = getParameterIndex(*tie);
  if (i < m_nOwnParams) {
    m_crystalField.addTie(tie);
  } else {
    checkSpectrumFunction();
    m_spectrum.addTie(tie);
  }
}

/// Returns the number of attributes associated with the function
size_t CrystalFieldSpectrum::nAttributes() const {
  return IFunction::nAttributes() + m_crystalField.nAttributes() +
         m_spectrum.nAttributes();
}

/// Returns a list of attribute names
std::vector<std::string> CrystalFieldSpectrum::getAttributeNames() const {
  std::vector<std::string> attNames = IFunction::getAttributeNames();
  auto cfNames = m_crystalField.getAttributeNames();
  auto spNames = m_spectrum.getAttributeNames();
  attNames.insert(attNames.end(), cfNames.begin(), cfNames.end());
  attNames.insert(attNames.end(), spNames.begin(), spNames.end());
  return attNames;
}

/// Return a value of attribute attName
API::IFunction::Attribute
CrystalFieldSpectrum::getAttribute(const std::string &attName) const {
  if (IFunction::hasAttribute(attName)) {
    return IFunction::getAttribute(attName);
  } else if (attName == "NumDeriv") {
    return m_spectrum.getAttribute(attName);
  } else if (isOwnName(attName)) {
    return m_crystalField.getAttribute(attName);
  } else {
    return m_spectrum.getAttribute(attName);
  }
}

/// Set a value to attribute attName
void CrystalFieldSpectrum::setAttribute(const std::string &attName,
                                        const IFunction::Attribute &att) {
  if (IFunction::hasAttribute(attName)) {
    IFunction::setAttribute(attName, att);
    m_dirty = true;
    m_spectrum.clear();
  } else if (attName == "NumDeriv") {
    m_spectrum.setAttribute(attName, att);
  } else if (isOwnName(attName)) {
    m_crystalField.setAttribute(attName, att);
    m_dirty = true;
  } else {
    checkSpectrumFunction();
    m_spectrum.setAttribute(attName, att);
  }
}

/// Check if attribute attName exists
bool CrystalFieldSpectrum::hasAttribute(const std::string &attName) const {
  if (attName == "NumDeriv" || IFunction::hasAttribute(attName)) {
    return true;
  }
  if (isOwnName(attName)) {
    return m_crystalField.hasAttribute(attName);
  } else {
    return m_spectrum.hasAttribute(attName);
  }
}

// Evaluates the function
void CrystalFieldSpectrum::function(const API::FunctionDomain &domain,
                                    API::FunctionValues &values) const {
  updateSpectrumFunction();
  m_spectrum.function(domain, values);
}

/// Test if a name (parameter's or attribute's) belongs to m_crystalFiled
/// @param aName :: A name to test.
bool CrystalFieldSpectrum::isOwnName(const std::string &aName) const {
  if (aName.empty()) {
    throw std::invalid_argument(
        "Parameter or attribute name cannot be empty string.");
  }
  return (aName.front() != 'f' || aName.find('.') == std::string::npos);
}

/// Uses m_crystalField to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldSpectrum::buildSpectrumFunction() const {
  m_dirty = false;
  m_spectrum.clear();

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_crystalField.functionGeneral(domain, values);

  if (values.size() == 0) {
    return;
  }

  if (values.size() % 2 != 0) {
    throw std::runtime_error(
        "CrystalFieldPeaks returned add number of values.");
  }

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto fwhm = IFunction::getAttribute("FWHM").asDouble();
  auto nPeaks = values.size() / 2;
  for (size_t i = 0; i < nPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    peak->fixCentre();
    peak->fixIntensity();
    peak->setCentre(values.getCalculated(i));
    peak->setIntensity(values.getCalculated(i + nPeaks));
    peak->setFwhm(fwhm);
    m_spectrum.addFunction(peak);
  }
}

/// Update m_spectrum function.
void CrystalFieldSpectrum::updateSpectrumFunction() const {
  if (m_spectrum.nFunctions() == 0) {
    buildSpectrumFunction();
    return;
  }
  m_dirty = false;
  FunctionDomainGeneral domain;
  FunctionValues values;
  m_crystalField.functionGeneral(domain, values);
  auto nPeaks = values.size() / 2;

  if (m_spectrum.nFunctions() != nPeaks) {
    // throw std::runtime_error("Number of peaks has changed:
    // "+std::to_string(nPeaks));
    buildSpectrumFunction();
    return;
  }

  for (size_t i = 0; i < nPeaks; ++i) {
    auto fun = m_spectrum.getFunction(i);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    auto centre = values.getCalculated(i);
    auto intensity = values.getCalculated(i + nPeaks);
    peak->setCentre(centre);
    peak->setIntensity(intensity);
  }
}

/// Update spectrum function if necessary.
void CrystalFieldSpectrum::checkSpectrumFunction() const {
  if (m_dirty) {
    updateSpectrumFunction();
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
