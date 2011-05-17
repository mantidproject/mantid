#ifndef MANTID_API_PARAMETERREFERENCE_H_
#define MANTID_API_PARAMETERREFERENCE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidAPI/IFitFunction.h"

namespace mu
{
  class Parser;
}

namespace Mantid
{
namespace API
{
/** 
    A reference to a parameter in a function. To uniquely identify a parameter
    in a composite function

    @author Roman Tolchenov, Tessella Support Services plc
    @date 26/02/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport ParameterReference
{
public:
  ParameterReference();
  ParameterReference(IFitFunction* fun, std::size_t index);
  IFitFunction* getFunction() const;
  std::size_t getIndex() const;
  void reset(IFitFunction* fun, std::size_t index);
  void setParameter(const double& value);
  double getParameter() const;

private:
  IFitFunction* m_function; ///< pointer to the function
  std::size_t m_index; ///< parameter index
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMETERREFERENCE_H_*/
