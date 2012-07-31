#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPV_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPV_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidKernel/Logger.h"
#include <complex>

namespace Mantid
{
namespace CurveFitting
{

  /** ThermalNeutronBk2BkExpConvPV :
      Back-to-back exponential convoluted with pseudo Voigt for thermal neutron and epithermal neutron TOF

      It will involve the calculation from peak's miller indices

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport ThermalNeutronBk2BkExpConvPV : virtual public API::IPeakFunction, virtual public API::IFunctionMW
{
  public:
    ThermalNeutronBk2BkExpConvPV();
    virtual ~ThermalNeutronBk2BkExpConvPV();

    /// overwrite IPeakFunction base class methods
    virtual double centre()const;
    virtual double height()const;
    virtual double fwhm()const;

    virtual void setHeight(const double h);

    /// overwrite IFunction base class methods
    std::string name()const{return "ThermalNeutronBk2BkExpConvPV";}
    virtual const std::string category() const { return "Peak";}

    /// Reset FWHM such that FWHM will be recalculated
    void resetFWHM();

    /// Set Miller Indicies
    void setMillerIndex(int h, int k, int l);

    /// Get Miller Index from this peak
    void getMillerIndex(int& h, int &k, int &l);

  protected:

    virtual void functionLocal(double* out, const double* xValues, const size_t nData)const;
    virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData);
    virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:

    /// Static reference to the logger class
    static Kernel::Logger& g_log;

    /// Integral for gamma
    std::complex<double> E1(std::complex<double> z) const;

    /// Calcualte H and Eta
    void calHandEta(double sigma2, double gamma, double& H, double& eta) const;

    /// Calcualte d-spacing value from Miller Indices for cubic
    double calCubicDSpace(double a, int h, int k, int l) const;

    /// Calculate peak center
    double calPeakCenter() const;

    /// Calculate peak profile I(TOF) = Omega(TOF)
    double calOmega(double x, double eta, double N, double alpha, double beta, double H,
        double sigma2, double invert_sqrt2sigma) const;

    /*
     * Set 2 functions to be hidden from client
     */
    virtual void setCentre(const double c);
    virtual void setFwhm(const double w);

    int mH;
    int mK;
    int mL;

    bool mHKLSet;
    
};

/// Shared pointer to ThermalNeutronBk2BkExpConvPV peak/function
typedef boost::shared_ptr<ThermalNeutronBk2BkExpConvPV> ThermalNeutronBk2BkExpConvPV_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPCONVPV_H_ */
