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
  declareParameter("PeakHeight", 0.0, "Hight of the peak");
  declareParameter("PeakCentre", 0.0, "Location of the peak");
  declareParameter("Width", 0.0, "Full width at half maximum");
  declareParameter("LeftShape", 1.0, "Left shape parameter");
  declareParameter("RightShape", 1.0, "Right shape parameter");

  // constrain shape parameters to the suggested range of values
  auto leftShapeConstraint = std::make_unique<BoundaryConstraint>(this, "LeftShape", 1.0, 20.0, true);
  addConstraint(std::move(leftShapeConstraint));
  auto rightShapeConstraint = std::make_unique<BoundaryConstraint>(this, "RightShape", 1.0, 20.0, true);
  addConstraint(std::move(rightShapeConstraint));
}

void AsymmetricPearsonVII::functionLocal(double *out, const double *xValues, const size_t nData) const {
  const double peak_height = getParameter("PeakHeight");
  const double peak_centre = getParameter("PeakCentre");
  const double weight = 1.0 / getParameter("Width");
  const double ml = getParameter("LeftShape");
  const double mr = getParameter("RightShape");
  double weight_sq = weight * weight;

  for (size_t i = 0; i < nData; ++i) {
    double offset = xValues[i] - peak_centre;
    double offset_sq = offset * offset;
    double left_part, right_part;

    if (ml == 0.0)
      left_part = peak_height / 2.0;
    else
      left_part = peak_height / pow(1.0 + 4.0 * offset_sq * (pow(2, 1.0 / ml) - 1.0) * weight_sq, ml);

    if (mr == 0.0)
      right_part = peak_height / 2.0;
    else
      right_part = peak_height / pow(1.0 + 4.0 * offset_sq * (pow(2, 1.0 / mr) - 1.0) * weight_sq, mr);

    if (xValues[i] <= peak_centre)
      out[i] = left_part;
    else
      out[i] = right_part;
  }
}

void AsymmetricPearsonVII::functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) {
  const double peak_height = getParameter("PeakHeight");
  const double peak_centre = getParameter("PeakCentre");
  const double weight = 1.0 / getParameter("Width");
  const double ml = getParameter("LeftShape");
  const double mr = getParameter("RightShape");
  double weight_sq = weight * weight;

  // derivatives
  for (size_t i = 0; i < nData; ++i) {
    double offset = xValues[i] - peak_centre;
    double offset_sq = offset * offset;
    double left_denom, right_denom;

    if (ml == 0.0)
      left_denom = 2.0;
    else
      left_denom = 1.0 + 4.0 * offset_sq * (pow(2, 1.0 / ml) - 1.0) * weight_sq;

    if (mr == 0.0)
      right_denom = 2.0;
    else
      right_denom = 1.0 + 4.0 * offset_sq * (pow(2, 1.0 / mr) - 1.0) * weight_sq;

    // derivative with respect to the peak_height
    double left_ph_der, right_ph_der;

    if (ml == 0.0)
      left_ph_der = 1.0 / left_denom;
    else
      left_ph_der = 1.0 / pow(left_denom, ml);

    if (mr == 0.0)
      right_ph_der = 1.0 / right_denom;
    else
      right_ph_der = 1.0 / pow(right_denom, mr);

    if (xValues[i] <= peak_centre)
      out->set(i, 0, left_ph_der);
    else
      out->set(i, 0, right_ph_der);

    // derivative with respect to the peak_centre
    double left_pc_der, right_pc_der;

    if (ml == 0.0)
      left_pc_der = 0.0;
    else
      left_pc_der = 8.0 * peak_height * ml * offset * (pow(2, 1.0 / ml) - 1.0) * weight_sq / pow(left_denom, 1.0 + ml);

    if (mr == 0.0)
      right_pc_der = 0.0;
    else
      right_pc_der =
          8.0 * peak_height * mr * offset * (pow(2, 1.0 / mr) - 1.0) * weight_sq / pow(right_denom, 1.0 + mr);

    if (xValues[i] <= peak_centre)
      out->set(i, 1, left_pc_der);
    else
      out->set(i, 1, right_pc_der);

    // derivative with respect to the weight, not width
    double left_weight_der, right_weight_der;
    if (ml == 0.0)
      left_weight_der = 0.0;
    else
      left_weight_der =
          -8.0 * peak_height * ml * offset_sq * (pow(2, 1.0 / ml) - 1.0) * weight / pow(left_denom, 1.0 + ml);

    if (mr == 0.0)
      right_weight_der = 0.0;
    else
      right_weight_der =
          -8.0 * peak_height * mr * offset_sq * (pow(2, 1.0 / mr) - 1.0) * weight / pow(right_denom, 1.0 + mr);

    if (xValues[i] <= peak_centre)
      out->set(i, 2, left_weight_der);
    else
      out->set(i, 2, right_weight_der);

    // derivative with respect to ml
    double ml_der;
    if (ml == 0.0)
      ml_der = -peak_height / 2.0 * log(4.0 * offset_sq * weight_sq);
    else
      ml_der = peak_height *
               (4.0 * offset_sq * weight_sq * pow(2, 1.0 / ml) * log(2.0) / ml / left_denom - log(left_denom)) /
               pow(left_denom, ml);

    if (xValues[i] <= peak_centre)
      out->set(i, 3, ml_der);
    else
      out->set(i, 3, 0.0);

    // derivative with respect to mr
    double mr_der;
    if (mr == 0.0)
      mr_der = -peak_height / 2.0 * log(4.0 * offset_sq * weight_sq);
    else
      mr_der = peak_height *
               (4.0 * offset_sq * weight_sq * pow(2, 1.0 / mr) * log(2.0) / mr / right_denom - log(right_denom)) /
               pow(right_denom, mr);

    if (xValues[i] <= peak_centre)
      out->set(i, 4, 0.0);
    else
      out->set(i, 4, mr_der);
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

} // namespace Mantid::CurveFitting::Functions
