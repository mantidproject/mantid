#ifndef MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILE_H_
#define MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILE_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/ComptonProfile.h"


namespace Mantid
{
namespace CurveFitting
{
  /**

    Implements a function to calculate the Compton profile of a nucleus using a Gram-Charlier approximation
    convoluted with an instrument resolution function that is approximated by a Voigt function.
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport GramCharlierComptonProfile : public ComptonProfile
  {
  public:
    /// Default constructor required by factory
    GramCharlierComptonProfile();
    
  private:
    /// A string identifier for this function
    std::string name() const;
    /// Declare the function parameters
    void declareParameters();
    /// Declare non-parameter attributes
    void declareAttributes();
    /// Set an attribute value (and possibly cache its value)
    void setAttribute(const std::string& name,const Attribute& value);
    /// Parse the active hermite polynomial coefficents
    void setHermiteCoefficients(const std::string & coeffs);
    /// Declare the Gram-Charlier (Hermite) coefficients
    void declareGramCharlierParameters();

    /// Returns the indices of the intensity parameters
    std::vector<size_t> intensityParameterIndices() const;
    /// Fill in the columns of the matrix for this mass
    size_t fillConstraintMatrix(Kernel::DblMatrix & cmatrix, const size_t index,const std::vector<double>& errors) const;
    /// Compute the sum for all Hermite polynomial coefficents
    void massProfile(double * result, const size_t nData) const;
    /// Compute the contribution to mass profile nth Hermite polynomial coefficient
    void addMassProfile(double * result, const unsigned int npoly) const;

    /// Add FSE term based on current parameter setting
    void addFSETerm(std::vector<double> & lhs) const;
    /// Convolute with resolution
    void convoluteVoigt(double * result, const size_t nData, const std::vector<double> & profile) const;
    /// Called by the framework when a workspace is set
    void setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wi,double startX, double endX);
    /// Pre-calculate the Y-space values
    void cacheYSpaceValues(const std::vector<double> & tseconds, const bool isHistogram,
                           const DetectorParams & detpar);

    /// The active hermite coefficents
    std::vector<short> m_hermite;
    ///Y values over a finer range
    std::vector<double> m_yfine;
    /// Interpolated Q values over a finer Y range
    std::vector<double> m_qfine;

    /// Holds the value of the Voigt function for each coarse y-space point as this is an expensive calculation
    std::vector<std::vector<double>> m_voigt;
    /// Holds the result Voigt multiplied by the profile function for the extended Y space range
    mutable std::vector<double> m_voigtProfile;

    /// Flag to hold whether the FSE parameter is fixed by the user
    bool m_userFixedFSE;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_GRAMCHARLIERCOMPTONPROFILE_H_ */
