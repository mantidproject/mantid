#ifndef MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILE_H_
#define MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILE_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/ComptonProfile.h"

namespace Mantid
{
namespace CurveFitting
{

  /**
    Implements a function to calculate the Compton profile of a nucleus using a Gaussian approximation
    convoluted with an instrument resolution function that is approximated by a Voigt function. The function calculates

    \f[\frac{E_0I(E_0)}{q}A_M J_M(y_M)\otimes R_M(t)\f]

    for the given mass where \f$J_M\f$ is approximated using a Gaussian and \f$R_M\f$ is the resolution function
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class MANTID_CURVEFITTING_DLL GaussianComptonProfile : public ComptonProfile
  {
  public:
    /// Default constructor required for factory
    GaussianComptonProfile();

  private:
    /// A string identifier for this function
    std::string name() const;
    /// Declare the function parameters
    void declareParameters();

    /// Returns the indices of the intensity parameters
    std::vector<size_t> intensityParameterIndices() const;
    /// Fill in the columns of the matrix for this mass
    size_t fillConstraintMatrix(Kernel::DblMatrix & cmatrix, const size_t start, const std::vector<double>& errors) const;

    /// Compute the function
    void massProfile(double * result, const size_t nData) const;

    /// Helper to allow the amplitude to be specified separately
    void massProfile(double * result, const size_t nData, const double amplitude) const;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_GAUSSIANCOMPTONPROFILE_H_ */
