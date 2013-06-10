#ifndef MANTID_CURVEFITTING_COMPTONPROFILE_H_
#define MANTID_CURVEFITTING_COMPTONPROFILE_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid
{
namespace CurveFitting
{

  /**
    This class serves as a base-class for ComptonProfile type functions. @see GaussianComptonProfile, GramCharlierComptonProfile
    
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
  class MANTID_CURVEFITTING_DLL ComptonProfile : public virtual API::ParamFunction, public virtual API::IFunction1D
  {
  public:
    /// Default constructor required for factory
    ComptonProfile();

    /** @name Function evaluation */
    ///@{
    /// Calculate the function
    void function1D(double* out, const double* xValues, const size_t nData) const;
    /// Ensure the object is ready to be fitted
    void setUpForFit();
    /// Cache a copy of the workspace pointer and pul out the parameters
    void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
    ///@}

    /// Returns the number of columns in the constraint matrix that are required for this mass
    virtual size_t numConstraintMatrixColumns() const = 0;
    /// Fill the appropriate columns of the given matrix with the values
    /// of the mass profile
    virtual void fillConstraintMatrix(Kernel::DblMatrix & cmatrix,const size_t start) = 0;

  protected:
    /// Declare parameters that will never participate in the fit
    void declareAttributes();
    /// Set an attribute value (and possibly cache its value)
    void setAttribute(const std::string& name,const Attribute& value);

    /// Override to calculate the value of the profile for this mass and store in the given array
    virtual void massProfile(double * result, const size_t nData) const = 0;

    /// Access y-values cache
    inline const std::vector<double> & ySpace() const { return m_yspace; }
    /// Access Q values cache
    inline const std::vector<double> & modQ() const { return m_modQ; }
    /// Access e0 values
    inline const std::vector<double> & e0() const { return m_e0; }
    /// Access the mass
    inline double mass() const { return m_mass; }
    /// Access total resolution width
    inline double resolutionFWHM() const { return m_resolutionSigma; }
    /// Access lorentz FWHM
    inline double lorentzFWHM() const { return m_lorentzFWHM; }

    /// Compute Voigt function interpolated around the given values
    void voigtApproxDiff(std::vector<double> & voigtDiff, const std::vector<double> & yspace, const double lorentzPos, const double lorentzAmp,
                         const double lorentzWidth, const double gaussWidth) const;
    /// Compute Voigt function
    void voigtApprox(std::vector<double> & voigt, const std::vector<double> & yspace, const double lorentzPos, const double lorentzAmp,
                     const double lorentzWidth, const double gaussWidth) const;

  protected:
    /// Logger
    Kernel::Logger & m_log;

  private:
    /// Retrieve a component parameter
    double getComponentParameter(const Geometry::IComponent & comp,const std::string &name) const;

    /// The workspace providing the data
    API::MatrixWorkspace_const_sptr m_workspace;
    /// Current workspace index, required to access instrument parameters
    size_t m_wsIndex;
    /// Store the mass values
    double m_mass;

    /// Source to sample distance
    double m_l1;
    /// Std dev of l1 distance
    double m_sigmaL1;
    /// Sample to detector distance
    double m_l2;
    /// Std dev of l1 distance
    double m_sigmaL2;
    /// Theta value for this spectrum
    double m_theta;
    /// Std Dev theta value for this spectrum
    double m_sigmaTheta;

    /// Final energy
    double m_e1;
    /// T0 value for this spectrum in seconds
    double m_t0;
    /// Lorentzian HWHM of the foil analyser energy
    double m_hwhmGaussE;
    /// Gaussian HWHM of the foil analyser energy
    double m_hwhmLorentzE;

    /// Voigt function
    boost::shared_ptr<API::IPeakFunction> m_voigt;

    /** @name Caches for commonly used values*/
    ///@{
    /// Y-values
    std::vector<double> m_yspace;
    /// Q-values
    std::vector<double> m_modQ;
    /// Incident energies
    std::vector<double> m_e0;
    /// Total resolution width
    double m_resolutionSigma;
    /// Lorentz FWHM
    double m_lorentzFWHM;
    ///@}

  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_COMPTONPROFILE_H_ */
