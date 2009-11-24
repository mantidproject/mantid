#ifndef MANTID_API_PARAMETERTIE_H_
#define MANTID_API_PARAMETERTIE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"

namespace mu
{
  class Parser;
}

namespace Mantid
{
namespace API
{
  class IFunction;
/** Ties fitting parameters. A tie is a formula that is used to 
    calculate the value of a function parameter based on the values of other parameters.
    A tied parameter is not considered independent and doesn't take part in fitting.
    Its value is always calculated with its tie's formula.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 28/10/2009

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport ParameterTie
{
public:
  /// Constructor
  ParameterTie(IFunction* funct,const std::string& parName);
  /// Destructor
  virtual ~ParameterTie();
  /// Set the tie expression
  virtual void set(const std::string& expr);
  /// Evaluate the expression
  virtual double eval();

  /// Get a pointer to the tied parameter.
  const double* parameter()const{return m_par;}
  /// Check if the tie has any references to certain parameters
  bool findParameters(const std::vector<const double*>& pars)const;

protected:
  mu::Parser* m_parser;         ///< math parser
  IFunction* m_function;        ///< Pointer to the function which parameter is to be tied
  double* m_par;                ///< Pointer to the tied parameter
  //int m_iPar;                   ///< index of the tied parameter

private:
  /// MuParser callback function
  static double* AddVariable(const char *varName, void *palg);
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMETERTIE_H_*/
