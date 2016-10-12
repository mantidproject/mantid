#include "MantidCurveFitting/Functions/CrystalFieldPeakUtils.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"

#include <algorithm>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
namespace CrystalFieldUtils {

using namespace CurveFitting;
using namespace Kernel;
using namespace API;

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
void setWidthConstraint(API::IPeakFunction& peak, double width, double fwhmVariation) {
  double upperBound = width + fwhmVariation;
  double lowerBound = width - fwhmVariation;
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


/// Populates a spectrum with peaks of type given by peakShape argument.
/// @param spectrum :: A composite function that is a collection of peaks.
/// @param peakShape :: A shape of each peak as a name of an IPeakFunction.
/// @param maxNPeaks :: A total number of peaks to add to spectrum. Not all
///        of them will be fitted.
/// @param nPeaks :: A number of peaks that will be actually fitted.
/// @param centresAndIntensities :: A FunctionValues object containing centres
///        and intensities for the peaks. First nPeaks calculated values are the
///        centres and the following nPeaks values are the intensities.
/// @param xVec :: x-values of a tabulated width function.
/// @param yVec :: y-values of a tabulated width function.
/// @param defaultFWHM :: A default value for the FWHM to use if xVec and yVec
///        are empty.
void buildSpectrumFunction(API::CompositeFunction &spectrum,
                           const std::string &peakShape, size_t maxNPeaks,
                           size_t nPeaks, const API::FunctionValues &centresAndIntensities,
                           const std::vector<double> &xVec,
                           const std::vector<double> &yVec,
                           double fwhmVariation, double defaultFWHM) {
  if (xVec.size() != yVec.size()) {
    throw std::runtime_error("WidthX and WidthY must have the same size.");
  }
  bool useDefaultFWHM = xVec.empty();
  for (size_t i = 0; i < maxNPeaks; ++i) {
    auto fun = API::FunctionFactory::Instance().createFunction(peakShape);
    auto peak = boost::dynamic_pointer_cast<API::IPeakFunction>(fun);
    if (!peak) {
      throw std::runtime_error("A peak function is expected.");
    }
    if (i < nPeaks) {
      auto centre = centresAndIntensities.getCalculated(i);
      peak->fixCentre();
      peak->fixIntensity();
      peak->setCentre(centre);
      peak->setIntensity(centresAndIntensities.getCalculated(i + nPeaks));
      if (useDefaultFWHM) {
        peak->setFwhm(defaultFWHM);
      } else {
        auto fwhm = calculateWidth(centre, xVec, yVec);
        peak->setFwhm(fwhm);
        setWidthConstraint(*peak, fwhm, fwhmVariation);
      }
    } else {
      peak->setHeight(0.0);
      peak->fixAll();
      peak->setFwhm(defaultFWHM);
    }
    spectrum.addFunction(peak);
  }
}

} // namespace CrystalFieldUtils
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
