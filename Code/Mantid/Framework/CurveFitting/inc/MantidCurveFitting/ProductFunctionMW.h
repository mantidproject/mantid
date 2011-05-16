#ifndef MANTID_CURVEFITTING_PRODUCTFUNCTIONMW_H_
#define MANTID_CURVEFITTING_PRODUCTFUNCTIONMW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunctionMW.h"
#include <boost/shared_array.hpp>
#include <cmath>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Allow user to create a fit function which is the product of two or
    more other fit functions.

    @author Anders Markvardsen, ISIS, RAL
    @date 4/4/2011

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
    class DLLExport ProductFunctionMW : public API::CompositeFunctionMW
    {
    public:
      /// Constructor
      ProductFunctionMW() {};
      /// Destructor
      ~ProductFunctionMW() {};

      /// overwrite IFunction base class methods
      std::string name()const{return "ProductFunctionMW";}
      void function(double* out, const double* xValues, const size_t nData)const;
      void functionDeriv(API::Jacobian* out, const double* xValues, const size_t nData);

    protected:
      /// overwrite IFunction base class method, which declare function parameters
      virtual void init() {};
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PRODUCTFUNCTIONMW_H_*/
