#ifndef MANTID_API_IBACKGROUNDFUNCTION_H_
#define MANTID_API_IBACKGROUNDFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionWithLocation.h"

namespace Mantid
{
namespace API
{
/** An interface to a background function. This interface is just
    a copy of the IFunctionWithLocation interface for now.

    @author Anders Markvardsen, ISIS, RAL
    @date 2/11/2009

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IBackgroundFunction : public IFunctionWithLocation
{
public:
  ///Fits the function
  /// @param X a vector of x values
  /// @param Y a matching vector of Y values
  virtual void fit(const std::vector<double>& X,const std::vector<double>& Y) = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IBACKGROUNDFUNCTION_H_*/
