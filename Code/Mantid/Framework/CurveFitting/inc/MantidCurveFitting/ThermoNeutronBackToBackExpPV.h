#ifndef MANTID_CURVEFITTING_THERMONEUTRONBACKTOBACKEXPPV_H_
#define MANTID_CURVEFITTING_THERMONEUTRONBACKTOBACKEXPPV_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidKernel/Logger.h"
#include <complex>

namespace Mantid
{
namespace CurveFitting
{

  /** ThermoNeutronBackToBackExpPV : Peak profile as the convolution of back-to-back exponential with pseudo-Voigt.
   for thermal neutron.
   It is peak profile No. 10 in Fullprof.
    
    @date 2012-06-06

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
  class DLLExport ThermoNeutronBackToBackExpPV : virtual public API::IPeakFunction, virtual public API::IFunctionMW
  {
  public:
    ThermoNeutronBackToBackExpPV();
    virtual ~ThermoNeutronBackToBackExpPV();
    
    /// overwrite IPeakFunction base class methods
    virtual double centre()const;
    virtual double height()const;
    virtual double fwhm()const;
    virtual void setCentre(const double c);
    virtual void setHeight(const double h);
    virtual void setFwhm(const double w);

    /// overwrite IFunction base class methods
    std::string name()const{return "ThermoNeutronBackToBackExpPV";}
    virtual const std::string category() const { return "Peak";}

    /// Set up the range of peak calculation for higher efficiency
    void setCalculationRange(double tof_low, double tof_upper);
    /// Calculate peak
    void geneatePeak(double* out, const double* xValues, const size_t nData);
    ///
    void resetFWHM();

  protected:

    virtual void functionLocal(double* out, const double* xValues, const size_t nData)const;
    virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData);
    virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:

    /// container for storing wavelength values for each data point
    mutable std::vector<double> m_dtt1;

    /// Static reference to the logger class
    static Kernel::Logger& g_log;

    double calOmega(double x, double eta, double N, double alpha, double beta, double H,
        double sigma2, double invert_sqrt2sigma) const;

    std::complex<double> E1(std::complex<double> z) const;

    void calHandEta(double sigma2, double gamma, double& H, double& eta) const;

    mutable double mFWHM;
    mutable double mLowTOF;
    mutable double mUpperTOF;

  };

  // typedef boost::shared_ptr<TableWorkspace> TableWorkspace_sptr;

  typedef boost::shared_ptr<ThermoNeutronBackToBackExpPV> ThermoNeutronBackToBackExpPV_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_THERMONEUTRONBACKTOBACKEXPPV_H_ */
