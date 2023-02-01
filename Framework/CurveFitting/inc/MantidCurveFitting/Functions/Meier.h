// Mantid Repository : https: // github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
// NScD Oak Ridge National Laboratory, European Spallation Source,
// Institut Laue - Langevin &CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier : GPL - 3.0 +
#pragma once
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"
#include <valarray>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
Time dependence of the polarization function for a static muon interacting with nuclear spin

Meier parameters:
<UL>
<LI> A0 - Amplitude (default 0.5)</LI>
<LI> FreqD - Frequency due to dipolar coupling (MHz) (default 0.01)</LI>
<LI> FreqQ - Frequency due to quadrupole interaction of the nuclear spin (MHz) (default 0.05)</LI>
<LI> Sigma - Gaussian decay rate (default 0.2)</LI>
<LI> Lambda - Exponential decay rate (default 0.1)</LI>
</UL>

Meier attributes:
<UL>
<LI> Spin - J, Total angular momentum quanutm number (default 3.5)</LI>
</UL>
*/
class MANTID_CURVEFITTING_DLL Meier : public API::ParamFunction, public API::IFunction1D {
public:
  std::string name() const override { return "Meier"; }
  const std::string category() const override { return "Muon\\MuonSpecific"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void init() override;

private:
  // Precomputes intermediate terms used to calculate the polrization in the x and z directions
  void precomputeIntermediateSteps(std::valarray<double> &cos2AlphaSquared, std::valarray<double> &negativeLambda,
                                   std::valarray<double> &positiveLambda, const double &J2) const;

  // Calculates and returns the value of cost 2*alpha sequared
  double getCos2AlphaSquared(const double &q1, const double &qq) const;

  // Calculates and returns the value of q1
  double getQ1(const double &m, const double &OmegaQ, const double &OmegaD) const;

  // Calculates and returns the value of q2
  double getQ2(const double &m, const double &J, const double &OmegaD) const;

  // Calculates and returns the value of q3
  double getQ3(const double &J, const double &OmegaQ, const double &OmegaD) const;

  // Calculates and returns the value of qq
  double getQQ(const double &q1, const double &q2) const;

  // Calculates and returns the value of positive lambda
  double getPositiveLambda(const double &q3, const double &Wm) const;

  // Calculates and returns the value of negative lambda
  double getNegativeLambda(const double &q3, const double &Wm) const;

  // Calculates and returns the value of lambda for the special cases
  double getBaseLambda(const double &OmegaQ, const double &OmegaD, const double &J) const;

  // Calculates the polarization in the x direction
  void calculatePx(std::valarray<double> &Px, const std::valarray<double> &xValues,
                   std::valarray<double> &cos2AlphaSquared, const std::valarray<double> &negativeLambda,
                   const std::valarray<double> &positiveLambda, const double &J2) const;

  // Calculates the polarization in the z direction
  void calculatePz(std::valarray<double> &Pz, const std::valarray<double> &xValues,
                   const std::valarray<double> &cos2AlphaSquared, const std::valarray<double> &negativeLambda,
                   const std::valarray<double> &positiveLambda, const double &J2) const;
};
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
