#ifndef MANTID_CURVEFITTING_PRODUCTQUADRATICEXP_H_
#define MANTID_CURVEFITTING_PRODUCTQUADRATICEXP_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
namespace CurveFitting
{

  /** ProductQuadraticExp : Function that evauates the product of an exponential and quadratic function.
    
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
  class DLLExport ProductQuadraticExp : public API::ParamFunction, public API::IFunction1D
  {
  public:
    ProductQuadraticExp();
    virtual ~ProductQuadraticExp();
    
    std::string name()const{return "ProductQuadraticExp";}

    virtual const std::string category() const { return "Calibrate";}

  protected:
    virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
    virtual void function1D(double* out, const double* xValues, const size_t nData) const;
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_PRODUCTQUADRATICEXP_H_ */