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
  Gram-Charlier approximation
  convoluted with an instrument resolution function that is approximated by a
  Voigt function.
*/
class MANTID_CURVEFITTING_DLL GramCharlierComptonProfile : public ComptonProfile {
public:
  /// Default constructor required by factory
  GramCharlierComptonProfile();

private:
  /// A string identifier for this function
  std::string name() const override;
  /// Declare the function parameters
  void declareParameters() override;
  /// Declare non-parameter attributes
  void declareAttributes() override;
  /// Set an attribute value (and possibly cache its value)
  void setAttribute(const std::string &name, const Attribute &value) override;
  /// Parse the active hermite polynomial coefficents
  void setHermiteCoefficients(const std::string &coeffs);
  /// Declare the Gram-Charlier (Hermite) coefficients
  void declareGramCharlierParameters();

  /// Returns the indices of the intensity parameters
  std::vector<size_t> intensityParameterIndices() const override;
  /// Fill in the columns of the matrix for this mass
  size_t fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                              const HistogramData::HistogramE &errors) const override;
  /// Compute the sum for all Hermite polynomial coefficents
  void massProfile(double *result, const size_t nData) const override;
  /// Compute the contribution to mass profile nth Hermite polynomial
  /// coefficient
  void addMassProfile(double *result, const unsigned int npoly) const;

  /// Add FSE term based on current parameter setting
  void addFSETerm(std::vector<double> &lhs) const;
  /// Convolute with resolution
  void convoluteVoigt(double *result, const size_t nData, const std::vector<double> &profile) const;
  /// Called by the framework when a workspace is set
  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;
  /// Pre-calculate the Y-space values
  void cacheYSpaceValues(const HistogramData::Points &tseconds, const Algorithms::DetectorParams &detpar) override;

  /// The active hermite coefficents
  std::vector<short> m_hermite;
  /// Y values over a finer range
  std::vector<double> m_yfine;
  /// Interpolated Q values over a finer Y range
  std::vector<double> m_qfine;

  /// Holds the value of the Voigt function for each coarse y-space point as
  /// this is an expensive calculation
  std::vector<std::vector<double>> m_voigt;
  /// Holds the result Voigt multiplied by the profile function for the extended
  /// Y space range
  mutable std::vector<double> m_voigtProfile;

  /// Flag to hold whether the FSE parameter is fixed by the user
  bool m_userFixedFSE;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
