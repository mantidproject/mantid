#ifndef MANTID_CURVEFITTING_DELTAFUNCTION_H_
#define MANTID_CURVEFITTING_DELTAFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"
#include <cmath>

namespace Mantid
{
  namespace CurveFitting
  {
    /**

    Delta function. Makes sence in Convolution only.

    @author Roman Tolchenov, Tessella plc
    @date 02/09/2010

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport DeltaFunction : public API::IPeakFunction
    {
    public:
      /// Constructor
      DeltaFunction();
      /// Destructor
      virtual ~DeltaFunction() {};


      /// overwrite IPeakFunction base class methods
      virtual double centre()const {return 0;}
      virtual double height()const {return getParameter("Height");}
      virtual double width()const {return 0;}
      virtual void setCentre(const double c) {}
      virtual void setHeight(const double h) {setParameter("Height",h);}
      virtual void setWidth(const double w) {}

      /// overwrite IFunction base class methods
      std::string name()const{return "DeltaFunction";}

    protected:
      virtual void function(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDeriv(API::Jacobian* out, const double* xValues, const size_t nData);
      virtual void functionLocal(double* out, const double* xValues, const size_t nData)const
      {
        UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
      }
      virtual void functionDerivLocal(API::Jacobian* out, const double* xValues, const size_t nData)
      {
        UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
      }
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_DELTAFUNCTION_H_*/
