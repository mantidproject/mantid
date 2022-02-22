// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Provides an implementation of the asymmetric PearsonVII function (sometimes it
  is also referred to as the split-PearsonVII function). This function combines
  two PearsonVII functions so to fit sharp peak curves. It is useful for analysis
  of X-ray diffractions peaks which consist of two separable components. The
  function takes 5 parameters: peak height (ph), peak center (pc), FWHM (w), left
  (ml) and right (mr) shape parameters.

  The function is defined as:

  AsymmetricPearsonVII(x, ph, pc, w, ml, mr) =
    PearsonVII(x, ph, pc, w, ml)*θ(pc-x) + PearsonVII(x, ph, pc, w, mr)*θ(x-pc),

  which is a superposition of left and right K-alpha peaks. θ denotes the Heavyside
  step function. The AsymmetricPearsonVII profile function is often used for peaks
  which are not strictly Gaussian or Lorentzian shaped. The parameters ml and mr
  determine (for the left and right side of the function, respectively) whether
  the function is more close to a Lorentzian or a Gaussian. When ml/mr=1 it's a
  Lorentzian, whereas when ml/mr->infinity it's a Gaussian.

  @author Oleksandr Koshchii, Forshungsztentrum Jülich at MLZ, Garching
  @date 18/02/2022
*/

class MANTID_CURVEFITTING_DLL AsymmetricPearsonVII : public API::IPeakFunction {
public:
  /// Constructor and destructor
  AsymmetricPearsonVII();

  /// Override IPeakFunction base class methods
  double height() const override;
  double centre() const override;
  double fwhm() const override;

  void setCentre(const double newCentre) override;
  void setHeight(const double newHight) override;
  void setFwhm(const double newFwhm) override;

  /// Override IFunction base class methods
  std::string name() const override { return "AsymmetricPearsonVII"; }
  const std::string category() const override { return "Peak; X-ray\\diffraction"; }

  double activeParameter(size_t i) const override;
  void setActiveParameter(size_t i, double value) override;

protected:
  void functionLocal(double *out, const double *xValues, const size_t nData) const override;
  void functionDerivLocal(API::Jacobian *out, const double *xValues, const size_t nData) override;
  /// Override IFunction base class method, which declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
