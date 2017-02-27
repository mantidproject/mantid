#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeakUtils.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidKernel/Strings.h"

#include <algorithm>
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(CrystalFieldSpectrum)

/// Constructor
CrystalFieldSpectrum::CrystalFieldSpectrum()
    : FunctionGenerator(IFunction_sptr(new CrystalFieldPeaks)), m_nPeaks(0) {
  declareAttribute("PeakShape", Attribute("Lorentzian"));
  declareAttribute("FWHM", Attribute(0.0));
  std::vector<double> vec;
  declareAttribute("FWHMX", Attribute(vec));
  declareAttribute("FWHMY", Attribute(vec));
  declareAttribute("FWHMVariation", Attribute(0.1));
  declareAttribute("NPeaks", Attribute(0));
  declareAttribute("FixAllPeaks", Attribute(false));
}

/// Uses m_crystalField to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldSpectrum::buildTargetFunction() const {
  m_dirty = false;
  auto spectrum = new CompositeFunction;
  m_target.reset(spectrum);
  m_target->setAttribute("NumDeriv", this->getAttribute("NumDeriv"));

  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);

  if (values.size() == 0) {
    return;
  }

  if (values.size() % 2 != 0) {
    throw std::runtime_error(
        "CrystalFieldPeaks returned odd number of values.");
  }

  auto xVec = getAttribute("FWHMX").asVector();
  auto yVec = getAttribute("FWHMY").asVector();
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();

  auto peakShape = getAttribute("PeakShape").asString();
  auto defaultFWHM = getAttribute("FWHM").asDouble();
  size_t nRequiredPeaks = getAttribute("NPeaks").asInt();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  m_nPeaks = CrystalFieldUtils::buildSpectrumFunction(
      *spectrum, peakShape, values, xVec, yVec, fwhmVariation, defaultFWHM,
      nRequiredPeaks, fixAllPeaks);
  storeReadOnlyAttribute("NPeaks", Attribute(static_cast<int>(m_nPeaks)));
}

/// Update m_spectrum function.
void CrystalFieldSpectrum::updateTargetFunction() const {
  if (!m_target) {
    buildTargetFunction();
    return;
  }
  m_dirty = false;
  auto peakShape = getAttribute("PeakShape").asString();
  auto xVec = getAttribute("FWHMX").asVector();
  auto yVec = getAttribute("FWHMY").asVector();
  auto fwhmVariation = getAttribute("FWHMVariation").asDouble();
  auto defaultFWHM = getAttribute("FWHM").asDouble();
  bool fixAllPeaks = getAttribute("FixAllPeaks").asBool();
  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);
  m_target->setAttribute("NumDeriv", this->getAttribute("NumDeriv"));
  auto &spectrum = dynamic_cast<CompositeFunction &>(*m_target);
  m_nPeaks = CrystalFieldUtils::calculateNPeaks(values);
  auto maxNPeaks = CrystalFieldUtils::calculateMaxNPeaks(m_nPeaks);
  CrystalFieldUtils::updateSpectrumFunction(
      spectrum, peakShape, values, m_nPeaks, 0, xVec, yVec, fwhmVariation,
      defaultFWHM, fixAllPeaks);
  storeReadOnlyAttribute("NPeaks", Attribute(static_cast<int>(m_nPeaks)));
}

/// Custom string conversion method
std::string CrystalFieldSpectrum::asString() const {
  std::ostringstream ostr;
  ostr << "name=" << this->name();
  // Print the attributes
  std::vector<std::string> attr = this->getAttributeNames();
  for (const auto &attName : attr) {
    std::string attValue = this->getAttribute(attName).value();
    if (!attValue.empty() && attValue != "\"\"" && attValue != "()") {
      ostr << ',' << attName << '=' << attValue;
    }
  }
  // Print own parameters
  for (size_t i = 0; i < m_nOwnParams; i++) {
    const ParameterTie *tie = getTie(i);
    if (!tie || !tie->isDefault()) {
      ostr << ',' << parameterName(i) << '=' << getParameter(i);
    }
  }

  // collect non-default constraints
  std::vector<std::string> constraints;
  for (size_t i = 0; i < m_nOwnParams; i++) {
    auto constraint = writeConstraint(i);
    if (!constraint.empty()) {
      constraints.push_back(constraint);
    }
  }

  // collect the non-default ties
  std::vector<std::string> ties;
  for (size_t i = 0; i < m_nOwnParams; i++) {
    auto tie = writeTie(i);
    if (!tie.empty()) {
      ties.push_back(tie);
    }
  }

  // Print parameters of the important peaks only
  const CompositeFunction &spectrum =
      dynamic_cast<const CompositeFunction &>(*m_target);
  size_t nPeakParams = 0;
  for (size_t ip = 0; ip < m_nPeaks; ++ip) {
    const auto &peak = dynamic_cast<IPeakFunction &>(*spectrum.getFunction(ip));
    // Print peak's atributes
    auto attr = peak.getAttributeNames();
    for (const auto &attName : attr) {
      std::string attValue = peak.getAttribute(attName).value();
      if (!attValue.empty() && attValue != "\"\"") {
        ostr << ",f" << ip << "." << attName << '=' << attValue;
      }
    }
    // Print peak's parameters
    for (size_t i = 0; i < peak.nParams(); i++) {
      ++nPeakParams;
      const ParameterTie *tie = peak.getTie(i);
      if (!tie || !tie->isDefault()) {
        ostr << ",f" << ip << "." << peak.parameterName(i) << '='
             << peak.getParameter(i);
      }
    }
  } // for peaks

  for (size_t i = 0; i < nPeakParams; ++i) {
      auto constraint = writeConstraint(i + m_nOwnParams);
      if (!constraint.empty()) {
        constraints.push_back(constraint);
      }
      auto tieStr = m_target->writeTie(i);
      if (!tieStr.empty()) {
        ties.push_back(tieStr);
      }
  }

  // print constraints
  if (!constraints.empty()) {
    ostr << ",constraints=("
         << Kernel::Strings::join(constraints.begin(), constraints.end(), ",")
         << ")";
  }

  // print the ties
  if (!ties.empty()) {
    ostr << ",ties=(" << Kernel::Strings::join(ties.begin(), ties.end(), ",")
         << ")";
  }

  return ostr.str();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
