#ifndef MANTID_CURVEFITTING_USERFUNCTION_H_
#define MANTID_CURVEFITTING_USERFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Function.h"
#include <boost/shared_array.hpp>

namespace mu
{
  class Parser;
}

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    A user defined function.

    @author Roman Tolchenov, Tessella plc
    @date 15/01/2010

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
    class DLLExport UserFunction : public API::Function
    {
    public:
      /// Constructor
      UserFunction();
      /// Destructor
      ~UserFunction();

      /// Returns the function's name
      std::string name()const{return "UserFunction";}

      /// Function you want to fit to.
      void function(double* out, const double* xValues, const int& nData)const;
      /// Derivatives of function with respect to active parameters
      void functionDeriv(API::Jacobian* out, const double* xValues, const int& nData);

      /// Returns the number of attributes associated with the function
      int nAttributes()const{return 1;}
      /// Returns a list of attribute names
      std::vector<std::string> getAttributeNames()const{return std::vector<std::string>(1,"Formula");}
      /// Return a value of attribute attName
      IFunction::Attribute getAttribute(const std::string& attName)const
      {
        return attName == "Formula"?IFunction::Attribute(m_formula):IFunction::getAttribute(attName);
      }
      /// Set a value to attribute attName
      void setAttribute(const std::string& attName,const IFunction::Attribute& value);
      /// Check if attribute attName exists
      bool hasAttribute(const std::string& attName)const{return attName == "Formula";}

    private:
      /// The formula
      std::string m_formula;
      /// muParser instance
      mu::Parser* m_parser;
      /// Used as 'x' variable in m_parser.
      mutable double m_x;
      /// True indicates that input formula contains 'x' variable
      bool m_x_set;
      /// Temporary data storage used in functionDeriv
      mutable boost::shared_array<double> m_tmp;
      /// Temporary data storage used in functionDeriv
      mutable boost::shared_array<double> m_tmp1;

      /// mu::Parser callback function for setting variables.
      static double* AddVariable(const char *varName, void *pufun);

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_USERFUNCTION_H_*/
