// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/AsymmetricPearsonVII.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"

#include <cmath>
#include <iostream>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
using namespace Constraints;
using namespace std;
using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(AsymmetricPearsonVII)

AsymmetricPearsonVII::AsymmetricPearsonVII() : IPeakFunction() {}

void AsymmetricPearsonVII::init() {
  declareParameter("PeakHeight", 1.0, "Hight of the peak");
  declareParameter("PeakCentre", 0.0, "Location of the peak");
  declareParameter("Width", 0.1, "Full width at half maximum");
  declareParameter("LeftShape", 1.0, "Left shape parameter");
  declareParameter("RightShape", 1.0, "Right shape parameter");

  // constrain shape parameters to the suggested range of values
  auto leftShapeConstraint = std::make_unique<BoundaryConstraint>(this, "LeftShape", 0.0, true);
  addConstraint(std::move(leftShapeConstraint));
  auto rightShapeConstraint = std::make_unique<BoundaryConstraint>(this, "RightShape", 0.0, true);
  addConstraint(std::move(rightShapeConstraint));
}

void AsymmetricPearsonVII::functionLocal(double *out, const double *xValues, const size_t nData) const {
  const double peak_height = getParameter("PeakHeight");
  const double peak_centre = getParameter("PeakCentre");
  const double weight = 1.0 / getParameter("Width");
  const double ml = getParameter("LeftShape");
  const double mr = getParameter("RightShape");

  for (size_t i = 0; i < nData; ++i) {
    double offset = xValues[i] - peak_centre;

    if (xValues[i] <= peak_centre) {
      // left_part
      if (ml == 0)
        out[i] = getPearsonVIILimitmEq0(peak_height);
      else
        out[i] = getPearsonVII(peak_height, offset, weight, ml);
    } else {
      // right_part
      if (mr == 0)
        out[i] = getPearsonVIILimitmEq0(peak_height);
      else
        out[i] = getPearsonVII(peak_height, offset, weight, mr);
    }
  }
}

void AsymmetricPearsonVII::functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
  const double peak_height = getParameter("PeakHeight");
  const double peak_centre = getParameter("PeakCentre");
  const double weight = 1.0 / getParameter("Width");
  const double ml = getParameter("LeftShape");
  const double mr = getParameter("RightShape");

  // derivatives
  for (size_t i = 0; i < nData; ++i) {
    double offset = xValues[i] - peak_centre;

    if (xValues[i] <= peak_centre) {
      // left_part
      if (ml == 0) {
        out->set(i, 0, getPearsonVIIDerivWRThLimitmEq0());
        out->set(i, 1, getPearsonVIIDerivWRTcLimitmEq0());
        out->set(i, 2, getPearsonVIIDerivWRTwLimitmEq0());
        out->set(i, 3, getPearsonVIIDerivWRTmLimitmEq0(peak_height, offset, weight));
      } else {
        out->set(i, 0, getPearsonVIIDerivWRTh(offset, weight, ml));
        out->set(i, 1, getPearsonVIIDerivWRTc(peak_height, offset, weight, ml));
        out->set(i, 2, getPearsonVIIDerivWRTw(peak_height, offset, weight, ml));
        out->set(i, 3, getPearsonVIIDerivWRTm(peak_height, offset, weight, ml));
      }
      out->set(i, 4, 0.0);
    } else {
      // right_part
      if (mr == 0) {
        out->set(i, 0, getPearsonVIIDerivWRThLimitmEq0());
        out->set(i, 1, getPearsonVIIDerivWRTcLimitmEq0());
        out->set(i, 2, getPearsonVIIDerivWRTwLimitmEq0());
        out->set(i, 4, getPearsonVIIDerivWRTmLimitmEq0(peak_height, offset, weight));
      } else {
        out->set(i, 0, getPearsonVIIDerivWRTh(offset, weight, mr));
        out->set(i, 1, getPearsonVIIDerivWRTc(peak_height, offset, weight, mr));
        out->set(i, 2, getPearsonVIIDerivWRTw(peak_height, offset, weight, mr));
        out->set(i, 4, getPearsonVIIDerivWRTm(peak_height, offset, weight, mr));
      }
      out->set(i, 3, 0.0);
    }
  }
}

void AsymmetricPearsonVII::setActiveParameter(size_t i, double value) {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Width")
    setParameter(i, 1.0 / value, false);
  else
    setParameter(i, value, false);
}

double AsymmetricPearsonVII::activeParameter(size_t i) const {
  if (!isActive(i)) {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Width")
    return 1.0 / getParameter(i);
  else
    return getParameter(i);
}

double AsymmetricPearsonVII::height() const { return getParameter("PeakHeight"); }
double AsymmetricPearsonVII::centre() const { return getParameter("PeakCentre"); }
double AsymmetricPearsonVII::fwhm() const { return getParameter("Width"); }
double AsymmetricPearsonVII::leftShape() const { return getParameter("LeftShape"); }
double AsymmetricPearsonVII::rightShape() const { return getParameter("RightShape"); }

void AsymmetricPearsonVII::setCentre(const double c) { setParameter("PeakCentre", c); }
void AsymmetricPearsonVII::setHeight(const double h) { setParameter("PeakHeight", h); }
void AsymmetricPearsonVII::setFwhm(const double w) { setParameter("Width", w); }
void AsymmetricPearsonVII::setLeftShape(const double ml) { setParameter("LeftShape", ml); }
void AsymmetricPearsonVII::setRightShape(const double mr) { setParameter("RightShape", mr); }

double AsymmetricPearsonVII::getPearsonVII(double peak_height, double offset, double weight, double m) const {
  return peak_height / pow(denominator_function(offset, weight, m), m);
}
double AsymmetricPearsonVII::getPearsonVIILimitmEq0(double peak_height) const { return peak_height / 2.0; }

double AsymmetricPearsonVII::getPearsonVIIDerivWRTh(double offset, double weight, double m) const {
  return 1.0 / pow(denominator_function(offset, weight, m), m);
}
double AsymmetricPearsonVII::getPearsonVIIDerivWRThLimitmEq0() const { return 0.5; }

double AsymmetricPearsonVII::getPearsonVIIDerivWRTc(double peak_height, double offset, double weight, double m) const {
  return derivative_function(peak_height, offset, weight, m) * weight;
}
double AsymmetricPearsonVII::getPearsonVIIDerivWRTcLimitmEq0() const { return 0.0; }

double AsymmetricPearsonVII::getPearsonVIIDerivWRTw(double peak_height, double offset, double weight, double m) const {
  return -derivative_function(peak_height, offset, weight, m) * offset;
}
double AsymmetricPearsonVII::getPearsonVIIDerivWRTwLimitmEq0() const { return 0.0; }

double AsymmetricPearsonVII::getPearsonVIIDerivWRTm(double peak_height, double offset, double weight, double m) const {
  return m_derivative_function(peak_height, offset, weight, m);
}
double AsymmetricPearsonVII::getPearsonVIIDerivWRTmLimitmEq0(double peak_height, double offset, double weight) const {
  double offset_sq = offset * offset;
  double weight_sq = weight * weight;
  return -peak_height / 2.0 * log(4.0 * offset_sq * weight_sq);
}

double denominator_function(double offset, double weight, double m) {
  double offset_sq = offset * offset;
  double weight_sq = weight * weight;
  return 1.0 + 4.0 * offset_sq * (pow(2, 1.0 / m) - 1.0) * weight_sq;
}

double derivative_function(double peak_height, double offset, double weight, double m) {
  return 8.0 * peak_height * m * offset * (pow(2, 1.0 / m) - 1.0) * weight /
         pow(denominator_function(offset, weight, m), 1.0 + m);
}

double m_derivative_function(double peak_height, double offset, double weight, double m) {
  double offset_sq = offset * offset;
  double weight_sq = weight * weight;
  return peak_height *
         (4.0 * offset_sq * weight_sq * pow(2, 1.0 / m) * log(2.0) / m / denominator_function(offset, weight, m) -
          log(denominator_function(offset, weight, m))) /
         pow(denominator_function(offset, weight, m), m);
}

} // namespace Mantid::CurveFitting::Functions
