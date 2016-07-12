#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParameterTie.h"

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

  size_t maxNPeaks = m_source->getAttribute("MaxPeakCount").asInt();

  if (values.size() == 0) {
    return;
  }

  if (values.size() % 2 != 0) {
    throw std::runtime_error(
        "CrystalFieldPeaks returned odd number of values.");
  }

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto fwhm = IFunction::getAttribute("FWHM").asDouble();
  auto nPeaks = values.size() / 2;
  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    if (i < nPeaks) {
      peak->fixCentre();
      peak->fixIntensity();
      peak->setCentre(values.getCalculated(i));
      peak->setIntensity(values.getCalculated(i + nPeaks));
    } else {
      peak->setHeight(0.0);
      peak->fixAll();
    }
    peak->setFwhm(fwhm);
    spectrum->addFunction(peak);
  }
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

  size_t maxNPeaks = m_source->getAttribute("MaxPeakCount").asInt();
  auto spectrum = dynamic_cast<CompositeFunction *>(m_target.get());
  if (!spectrum || spectrum->nFunctions() != maxNPeaks) {
    buildTargetFunction();
    return;
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
