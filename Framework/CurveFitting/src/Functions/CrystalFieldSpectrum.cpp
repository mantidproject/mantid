#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeakUtils.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"

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
  declareAttribute("WidthX", Attribute(vec));
  declareAttribute("WidthY", Attribute(vec));
  declareAttribute("WidthVariation", Attribute(0.1));
}

/// Uses m_crystalField to calculate peak centres and intensities
/// then populates m_spectrum with peaks of type given in PeakShape attribute.
void CrystalFieldSpectrum::buildTargetFunction() const {
  m_dirty = false;
  auto spectrum = new CompositeFunction;
  m_target.reset(spectrum);

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

  auto xVec = IFunction::getAttribute("WidthX").asVector();
  auto yVec = IFunction::getAttribute("WidthY").asVector();
  auto fwhmVariation = getAttribute("WidthVariation").asDouble();

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto defaultFWHM = IFunction::getAttribute("FWHM").asDouble();
  m_nPeaks = CrystalFieldUtils::buildSpectrumFunction(*spectrum, peakShape, values, xVec,
                                           yVec, fwhmVariation, defaultFWHM);
}

/// Update m_spectrum function.
void CrystalFieldSpectrum::updateTargetFunction() const {
  if (!m_target) {
    buildTargetFunction();
    return;
  }
  m_dirty = false;
  FunctionDomainGeneral domain;
  FunctionValues values;
  m_source->function(domain, values);
  auto &spectrum = dynamic_cast<CompositeFunction&>(*m_target);
  m_nPeaks = CrystalFieldUtils::updateSpectrumFunction(spectrum, values, m_nPeaks);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
