#ifndef MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_
#define MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include <complex>

namespace Mantid
{
namespace CurveFitting
{

  /** Bk2BkExpConvPV : Peak profile as tback-to-back exponential convoluted with pseudo-Voigt.
    It is peak profile No. 10 in Fullprof.
    
    @date 2012-06-06

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport Bk2BkExpConvPV : virtual public API::IPeakFunction, virtual public API::IFunctionMW
  {
  public:
    Bk2BkExpConvPV();
    virtual ~Bk2BkExpConvPV();
    
    /// overwrite IPeakFunction base class methods
    virtual double centre()const;
    virtual double height()const;
    virtual double fwhm()const;
    virtual void setCentre(const double c);
    virtual void setHeight(const double h);
    virtual void setFwhm(const double w);

    /// overwrite IFunction base class methods
    std::string name()const{return "Bk2BkExpConvPV";}
    virtual const std::string category() const { return "Peak";}

    /// Set up the range of peak calculation for higher efficiency
    // void setCalculationRange(double tof_low, double tof_upper);
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

    double calOmega(double x, double eta, double N, double alpha, double beta, double H,
        double sigma2, double invert_sqrt2sigma) const;

    std::complex<double> E1(std::complex<double> z) const;

    void calHandEta(double sigma2, double gamma, double& H, double& eta) const;

    mutable double mFWHM;
    mutable double mLowTOF;
    mutable double mUpperTOF;

  };

  // typedef boost::shared_ptr<TableWorkspace> TableWorkspace_sptr;

  typedef boost::shared_ptr<Bk2BkExpConvPV> Bk2BkExpConvPV_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_BK2BKEXPCONVPV_H_ */
