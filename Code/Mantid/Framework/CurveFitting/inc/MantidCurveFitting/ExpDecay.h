#ifndef MANTID_CURVEFITTING_EXPDECAY_H_
#define MANTID_CURVEFITTING_EXPDECAY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Provide exponential decay function: h*exp(-(x-c)/t)

    @author Roman Tolchenov, Tessella plc
    @date 05/11/2010

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport ExpDecay : public API::ParamFunction, public API::IFunction1D
    {
    public:
      /// Constructor
      ExpDecay();
      /// Destructor
      virtual ~ExpDecay() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "ExpDecay";}

      /// overwrite IFunction base class methods
      virtual const std::string category() const { return "General";}
    protected:
      virtual void function1D(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_EXPDECAY_H_*/
