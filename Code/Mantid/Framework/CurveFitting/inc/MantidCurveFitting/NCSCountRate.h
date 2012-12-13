#ifndef MANTID_CURVEFITTING_NCSCOUNTRATE_H_
#define MANTID_CURVEFITTING_NCSCOUNTRATE_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {

    /** NCSCountRate

    The count rate for Neutron Compton Scattering can be approximated by:

    \f[C_m(t) = \frac{E_0I(E_0)}{q}\sum_M A_M J_M(y_M)\otimes R_M(t)\f]

    This Fit function implements the above formula so it can be fitted to time-of-flight data. It is
    described in more detail here: http://jcp.aip.org/resource/1/jcpsa6/v134/i11/p114511_s1 and is mainly intended
    for use with the VESUVIO instrument at ISIS.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_CURVEFITTING_DLL NCSCountRate : public virtual API::ParamFunction, public virtual API::IFunction1D
    {
    public:
      /// Default constructor required for factory
      NCSCountRate();

    private:
      /// Name of the function in the factory
      std::string name() const;

      /** @name Function setup */
      ///@{
      /// Declare parameters that will never participate in the fit
      void declareAttributes();
      /// Declare fitting parameters that could vary during the fitting process.
      void declareParameters();
      /// Set an attribute value (and possibly cache its value)
      void setAttribute(const std::string& name,const Attribute& value);
      /// Cache a copy of the workspace pointer
      void setWorkspace(boost::shared_ptr<const API::Workspace> ws);
      /// Ensure the object is ready to be fitted
      void setUpForFit();
      ///@}

      /** @name Function evaluation */
      ///@{
      void function1D(double* out, const double* xValues, const size_t nData) const;
      ///@}

      /** @name Attribute setters */
      ///@{
      /// Sets the masses involved in the fit.
      void setMasses(const std::string & nmasses);
      /// Declare the Gaussian parameters
      void declareGaussianParameters();
      /// Parse the active hermite polynomial coefficents
      void setHermiteCoefficients(const std::string & coeffs);
      /// Declare the Gram-Charlier (Hermite) coefficients
      void declareGramCharlierParameters();
      /// Set the degree of polynomial for the background
      void setBackgroundPolyDegree(const int npoly);
      ///@}

      /** @name Attribute query */
      ///@{
      /// Query if the background is included
      bool backgroundRequsted() const;
      ///@}

      /** @name Helpers */
      ///@{
      /// Retrieve a component parameter
      double getComponentParameter(const Geometry::IComponent & comp,const std::string &name) const;
      ///@}

      /// The workspace providing the data
      API::MatrixWorkspace_const_sptr m_workspace;
      /// Current workspace index, required to access instrument parameters
      size_t m_wsIndex;
      /// Store the mass values
      std::vector<double> m_masses;
      /// Store the values of whether the Hermite polynomial coefficents are active
      std::vector<short> m_hermite;
      /// The degree of background to incorporate
      int m_bkgdPoly;

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
      /// T0 value for this spectrum
      double m_t0;
      /// Lorentzian HWHM of the foil analyser energy
      double m_hwhmGaussE;
      /// Gaussian HWHM of the foil analyser energy
      double m_hwhmLorentzE;
    };


  } // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_NCSCOUNTRATE_H_ */
