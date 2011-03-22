#ifndef MANTID_CURVEFITTING_CHEBYSHEV_H_
#define MANTID_CURVEFITTING_CHEBYSHEV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackgroundFunction.h"
#include <valarray>

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Implements Chebyshev polynomial expansion.

    Attributes: int n - the highest polynomial order.
    Parameters: n+1 expansion coefficients a_i as in expression:
    Sum_i=0^n a_i * T_i(x)

    Uses the Clenshaw algorithm to evaluate the expansion.

    @author Roman Tolchenov, Tessella inc
    @date 14/05/2010

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
    class DLLExport Chebyshev : public BackgroundFunction
    {
    public:
      /// Constructor
      Chebyshev();
      /// Destructor
      ~Chebyshev() {};

      /// overwrite IFunction base class methods
      std::string name()const{return "Chebyshev";}
      void function(double* out, const double* xValues, const int& nData)const;
      void functionDeriv(API::Jacobian* out, const double* xValues, const int& nData);

      /// Returns the number of attributes associated with the function
      int nAttributes()const{return 1;}
      /// Returns a list of attribute names
      std::vector<std::string> getAttributeNames()const;
      /// Return a value of attribute attName
      Attribute getAttribute(const std::string& attName)const;
      /// Set a value to attribute attName
      void setAttribute(const std::string& attName,const Attribute& );
      /// Check if attribute attName exists
      bool hasAttribute(const std::string& attName)const;

    private:

      /// Polynomial order
      int m_n;
      /// Lower x boundary. The default is -1
      double m_StartX;
      /// Upper x boundary. The default is  1
      double m_EndX;
      /// Keep intermediate calculatons
      mutable std::valarray<double> m_b;

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CHEBYSHEV_H_*/
