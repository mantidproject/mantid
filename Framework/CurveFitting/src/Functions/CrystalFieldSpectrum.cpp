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
    : FunctionGenerator(IFunction_sptr(new CrystalFieldPeaks)) {
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
  auto nPeaks = values.size() / 2;
  size_t maxNPeaks = nPeaks + nPeaks / 2 + 1;
  CrystalFieldUtils::buildSpectrumFunction(*spectrum, peakShape, maxNPeaks, nPeaks, values, xVec,
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
  auto nPeaks = values.size() / 2;

  size_t maxNPeaks = nPeaks + nPeaks / 2 + 1;
  auto spectrum = dynamic_cast<CompositeFunction *>(m_target.get());
  if (!spectrum) {
    buildTargetFunction();
    return;
  }
  if (spectrum->nFunctions() != maxNPeaks) {
    maxNPeaks = spectrum->nFunctions();
    if (nPeaks > maxNPeaks) {
      nPeaks = maxNPeaks;
    }
  }

  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = spectrum->getFunction(i);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    if (i < nPeaks) {
      auto centre = values.getCalculated(i);
      auto intensity = values.getCalculated(i + nPeaks);
      peak->setCentre(centre);
      peak->setIntensity(intensity);
    } else {
      peak->setHeight(0.0);
      peak->fixAll();
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
