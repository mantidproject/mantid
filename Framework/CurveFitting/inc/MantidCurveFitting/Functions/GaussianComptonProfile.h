// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/ComptonProfile.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Implements a function to calculate the Compton profile of a nucleus using a
  Gaussian approximation
  convoluted with an instrument resolution function that is approximated by a
  Voigt function. The function calculates

  \f[\frac{E_0I(E_0)}{q}A_M J_M(y_M)\otimes R_M(t)\f]

  for the given mass where \f$J_M\f$ is approximated using a Gaussian and
  \f$R_M\f$ is the resolution function
*/
class MANTID_CURVEFITTING_DLL GaussianComptonProfile : public ComptonProfile {
public:
  /// Default constructor required for factory
  GaussianComptonProfile();

private:
  /// A string identifier for this function
  std::string name() const override;
  /// Declare the function parameters
  void declareParameters() override;

  /// Returns the indices of the intensity parameters
  std::vector<size_t> intensityParameterIndices() const override;
  /// Fill in the columns of the matrix for this mass
  size_t fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                              const HistogramData::HistogramE &errors) const override;

  /// Compute the function
  void massProfile(double *result, const size_t nData) const override;

  /// Helper to allow the amplitude to be specified separately
  void massProfile(double *result, const size_t nData, const double amplitude) const;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
