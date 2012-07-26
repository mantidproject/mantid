#ifndef MANTID_CURVEFITTING_HIGHORDERPOLYNOMIALBACKGROUND_H_
#define MANTID_CURVEFITTING_HIGHORDERPOLYNOMIALBACKGROUND_H_

#include "MantidKernel/System.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

  /** HighOrderPolynomialBackground : TODO: DESCRIPTION
    
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
  class DLLExport HighOrderPolynomialBackground : public BackgroundFunction
  {
  public:
    HighOrderPolynomialBackground();
    virtual ~HighOrderPolynomialBackground();

    std::string name()const{return "HighOrderPolynomialBackground";}
    virtual void function1D(double* out, const double* xValues, const size_t nData)const;
    virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);

  private:
    void init();
    
  };


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_HIGHORDERPOLYNOMIALBACKGROUND_H_ */
