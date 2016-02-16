#ifndef MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_
#define MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_

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
  /// Default constructor required for factory
  MultivariateGaussianComptonProfile();

private:
  /// A string identifier for this function
  std::string name() const;
  /// Declare the function parameters
  void declareParameters();
  /// Declare parameters that will never participate in the fit
  void declareAttributes();
  /// Set an attribute value (and possibly cache its value)
  void setAttribute(const std::string &name, const Attribute &value);

  /// Returns the indices of the intensity parameters
  std::vector<size_t> intensityParameterIndices() const;
  /// Fill in the columns of the matrix for this mass
  size_t fillConstraintMatrix(Kernel::DblMatrix &cmatrix, const size_t start,
                              const std::vector<double> &errors) const;

  /// Compute the function
  void massProfile(double *result, const size_t nData) const;

  double integratePhi(int idx, std::vector<double> &s2Cache, double y) const;
  double calculateIntegrand(int idx, std::vector<double> &s2Cache,
                            double y) const;

  void buildS2Cache(std::vector<double> &s2Cache) const;

  int m_integrationSteps;
  double m_thetaStep;
  double m_phiStep;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_MULTIVARIATEGAUSSIANCOMPTONPROFILE_H_ */
