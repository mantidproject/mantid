#ifndef MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid
{
namespace CurveFitting
{

  /** ThermalNeutronDtoTOFFunction : TODO: DESCRIPTION
    
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
  class DLLExport ThermalNeutronDtoTOFFunction : virtual public API::IFunction1D, public API::ParamFunction
  {
  public:
    ThermalNeutronDtoTOFFunction();
    virtual ~ThermalNeutronDtoTOFFunction();

    /// Override
    virtual void function1D(double* out, const double* xValues, const size_t nData) const;

    /// overwrite IFunction base class methods
    std::string name()const{return "ThermalNeutronDtoTOFFunction";}

    /// Overwrite IFunction
    virtual const std::string category() const { return "General";}

  protected:
    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:
    /// Core function (inline) to calcualte TOF_h from d-spacing
    inline double corefunction(double dh, double dtt1, double dtt1t, double dtt2t,
                               double zero, double zerot, double width, double tcross) const;
    
  };

  typedef boost::shared_ptr<ThermalNeutronDtoTOFFunction> ThermalNeutronDtoTOFFunction_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_ */
