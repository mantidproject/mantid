#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMA_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMA_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid
{
namespace CurveFitting
{

  /** ThermalNeutronBk2BkExpSIGMA : Function to calculate Sigma of Bk2Bk Exponential function from
    Thermal Neutron Function's Sig0, Sig1, Sig2, Width and etc.
    
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport ThermalNeutronBk2BkExpSigma : virtual public API::IFunction1D, public API::ParamFunction
  {
  public:
    ThermalNeutronBk2BkExpSigma();
    virtual ~ThermalNeutronBk2BkExpSigma();

    /// Override
    virtual void function1D(double* out, const double* xValues, const size_t nData) const;

    /// overwrite IFunction base class methods
    std::string name()const{return "ThermalNeutronBk2BkExpSigma";}

    /// Overwrite IFunction
    virtual const std::string category() const { return "General";}

  protected:
    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:
    /// Derivative
    void functionDerivLocal(API::Jacobian* , const double* , const size_t );

    /// Derivative to overwrite
    void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// Core function (inline) to calcualte TOF_h from d-spacing
    inline double corefunction(double dh, double sig0sq, double sig1sq, double sig2sq) const;
    
  };

  typedef boost::shared_ptr<ThermalNeutronBk2BkExpSigma> ThermalNeutronBk2BkExpSigma_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPSIGMA_H_ */
