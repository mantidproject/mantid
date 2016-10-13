#ifndef MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_
#define MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_

#include <cmath>

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/ComptonProfile.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_CURVEFITTING_DLL MultivariateGaussianComptonProfile
    : public ComptonProfile {
public:
  static const char *AMP_PARAM;
  static const char *SIGMA_X_PARAM;
  static const char *SIGMA_Y_PARAM;
  static const char *SIGMA_Z_PARAM;
  static const char *STEPS_ATTR;

  /// Default constructor required for factory
  MultivariateGaussianComptonProfile();

  void buildS2Cache(std::vector<double> &s2Cache) const;

private:
  /// A string identifier for this function
  std::string name() const override;
  /// Declare the function parameters
  void declareParameters() override;
  /// Declare parameters that will never participate in the fit
  void declareAttributes() override;
  /// Set an attribute value (and possibly cache its value)
  void setAttribute(const std::string &name, const Attribute &value) override;

  /// Returns the indices of the intensity parameters
  std::vector<size_t> intensityParameterIndices() const override;
  /// Fill in the columns of the matrix for this mass
  size_t
  fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                       const HistogramData::HistogramE &errors) const override;

  /// Compute the function
  void massProfile(double *result, const size_t nData) const override;

  /// Helper to allow the amplitude to be specified separately
  void massProfile(double *result, const size_t nData,
                   const double amplitude) const;

  double calculateJ(std::vector<double> s2Cache, double y) const;
  double calculateFSE(std::vector<double> s2Cache, double y) const;

  /**
   * @brief Calculates the integrand of the mass profile.
   * @param s2 S2 value
   * @param y Y value
   * @return Integrand
   */
  inline double calculateIntegrandJ(double s2, double y) const {
    return s2 * exp(-(y * y) / (2.0 * s2));
  }

  /**
   * @brief Calculates the integrand of the A3 FSE correction.
   * @param s2 S2 value
   * @param y Y value
   * @return Integrand
   */
  inline double calculateIntegrandFSE(double s2, double y) const {
    return ((pow(y, 3) / pow(s2, 4)) - ((3 * y) / pow(s2, 2))) *
           exp(-(y * y) / (2.0 * s2));
  }

  double intervalCoeff(int i, int j) const;

  int m_integrationSteps; //!< Number of steps to perform during integration
  double m_thetaStep;     //!< Delta theta in integration
  double m_phiStep;       //!< Delta phi in integration
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_ */
