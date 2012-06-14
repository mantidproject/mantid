#ifndef MANTID_CURVEFITTING_LEBAILFUNCTION_H_
#define MANTID_CURVEFITTING_LEBAILFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid
{
namespace CurveFitting
{

  /** LeBailFunction : LeBail Fit
   *
    Prototype:  mainly focussed on the workflow
    
    First goal: Fit 2 peaks

    @date 2012-06-11

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
  class DLLExport LeBailFunction : public API::ParamFunction, public API::IFunction1D, public API::IFunctionMW
  {
  public:
    LeBailFunction();
    virtual ~LeBailFunction();
    
    virtual std::string name() const;

    // Functions to input parameters
    void setPeak(double d, double height);

    void calPeaks(double* out, const double* xValues, const size_t nData);

    double getPeakParameter(size_t index, std::string parname) const;

  protected:

    virtual void function1D(double* out, const double* xValues, const size_t nData)const;
    virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
    virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:

    static Kernel::Logger& g_log;

    void calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h, double &sigma_g2, double &gamma_l, std::map<std::string, double>& parmap) const;
    void adPeakPositionD(double dh);

    double mL1;
    double mL2;

    mutable double Alph0, Alph1, Alph0t, Alph1t;
    mutable double Beta0, Beta1, Beta0t, Beta1t;
    mutable double Sig0, Sig1, Sig2, Gam0, Gam1, Gam2;
    mutable double Dtt1, Dtt2, Dtt1t, Dtt2t, Zero, Zerot;

    std::vector<double> dvalues;
    std::vector<double> heights;
    mutable std::vector<std::map<std::string, double> > mPeakParameters; // It is in strict order with dvalues;

    API::IPeakFunction* mPeak;

  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_LEBAILFUNCTION_H_ */
