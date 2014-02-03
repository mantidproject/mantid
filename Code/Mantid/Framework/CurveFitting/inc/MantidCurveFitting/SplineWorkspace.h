#ifndef MANTID_CURVEFITTING_SplineWorkspace_H_
#define MANTID_CURVEFITTING_SplineWorkspace_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/IFunction1D.h"

#include <gsl/gsl_sf_erf.h>
#include <cmath>

using namespace std;

namespace Mantid
{
namespace CurveFitting
{

  /** SplineWorkspace : function to interpolate workspace data
    
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
  class DLLExport SplineWorkspace :  public API::ParamFunction, public virtual API::IFunction1D, public virtual API::IFunctionMW
  {
  public:
    SplineWorkspace();
    virtual ~SplineWorkspace();

    /// Override
    virtual void function1D(double* out, const double* xValues, const size_t nData) const;

    /// overwrite IFunction base class methods
    std::string name()const{return "SplineWorkspace";}

    /// Overwrite IFunction
    virtual const std::string category() const { return "General";}

  protected:
    /// overwrite IFunction base class method, which declare function parameters
    virtual void init();

  private:

    /// Derviate to overwritten
    void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData);

  };

  typedef boost::shared_ptr<SplineWorkspace> SplineWorkspace_sptr;

} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_SplineWorkspace_H_ */
