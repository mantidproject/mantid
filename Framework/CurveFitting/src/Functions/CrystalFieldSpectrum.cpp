#include "MantidCurveFitting/Functions/CrystalFieldSpectrum.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaks.h"

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

namespace {

/// Calculate the width of a peak cenrted at x using 
/// an interpolated value of a function tabulated at xVec points
/// @param x :: Peak centre.
/// @param xVec :: x-values of a tabulated width function.
/// @param yVec :: y-values of a tabulated width function.
double calculateWidth(double x, const std::vector<double> &xVec,
                      const std::vector<double> &yVec) {
  assert(xVec.size() == yVec.size());
  auto upperIt = std::lower_bound(xVec.begin(), xVec.end(), x);
  if (upperIt == xVec.end() || x < xVec.front()) {
    throw std::runtime_error("Cannot calculate peak width: peak at " +
                             std::to_string(x) + " is outside given range (" +
                             std::to_string(xVec.front()) + " : " +
                             std::to_string(xVec.back()) + ")");
  }
  if (upperIt == xVec.begin()) {
    return yVec.front();
  }
  double lowerX = *(upperIt - 1);
  double upperX = *upperIt;
  auto i = std::distance(xVec.begin(), upperIt) - 1;
  return yVec[i] + (yVec[i + 1] - yVec[i]) / (upperX - lowerX) * (x - lowerX);
}

/// Set a boundary constraint on the appropriate parameter of the peak.
/// @param peak :: A peak function.
/// @param width :: A width of the peak.
/// @param widthVariation :: A value by which the with can vary on both sides.
void setWidthConstraint(API::IPeakFunction& peak, double width, double widthVariation) {
  double upperBound = width + widthVariation;
  double lowerBound = width - widthVariation;
  bool fix = lowerBound == upperBound;
  if (!fix) {
    if (lowerBound < 0.0) {
      lowerBound = 0.0;
    }
    if (lowerBound >= upperBound) {
      lowerBound = upperBound / 2;
    }
  }
  if (peak.name() == "Lorentzian") {
    if (fix) {
      peak.fixParameter("FWHM");
      return;
    }
    auto constraint = new Constraints::BoundaryConstraint(&peak, "FWHM", lowerBound, upperBound);
    peak.addConstraint(constraint);
  } else if (peak.name() == "Gaussian") {
    if (fix) {
      peak.fixParameter("Sigma");
      return;
    }
    const double WIDTH_TO_SIGMA = 2.0 * sqrt(2.0 * M_LN2);
    lowerBound /= WIDTH_TO_SIGMA;
    upperBound /= WIDTH_TO_SIGMA;
    auto constraint = new Constraints::BoundaryConstraint(&peak, "Sigma", lowerBound, upperBound);
    peak.addConstraint(constraint);
  } else {
    throw std::runtime_error("Cannot set constraint on width of " + peak.name());
  }
}

} // namespace

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
  if (xVec.size() != yVec.size()) {
    throw std::runtime_error("WidthX and WidthY must have the same size.");
  }
  auto widthVariation = getAttribute("WidthVariation").asDouble();
  bool useDefaultWidth = xVec.empty();

  auto peakShape = IFunction::getAttribute("PeakShape").asString();
  auto defaultFwhm = IFunction::getAttribute("FWHM").asDouble();
  auto nPeaks = values.size() / 2;
  size_t maxNPeaks = nPeaks + nPeaks / 2 + 1;
  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    if (i < nPeaks) {
      auto centre = values.getCalculated(i);
      peak->fixCentre();
      peak->fixIntensity();
      peak->setCentre(centre);
      peak->setIntensity(values.getCalculated(i + nPeaks));
      if (useDefaultWidth) {
        peak->setFwhm(defaultFwhm);
      } else {
        auto fwhm = calculateWidth(centre, xVec, yVec);
        peak->setFwhm(fwhm);
        setWidthConstraint(*peak, fwhm, widthVariation);
      }
    } else {
      peak->setHeight(0.0);
      peak->fixAll();
      peak->setFwhm(defaultFwhm);
    }
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
