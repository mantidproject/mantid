#ifndef MANTID_CURVEFITTING_COMPOSITEVALUES_H_
#define MANTID_CURVEFITTING_COMPOSITEVALUES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionValues.h"

#include <vector>

namespace Mantid
{
namespace CurveFitting
{
/** Composite values.

    @author Roman Tolchenov, Tessella plc
    @date 10/04/2012

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport CompositeValues : public API::IFunctionValues
{
public:

protected:
  std::vector<API::IFunctionValues_sptr> m_values;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COMPOSITEVALUES_H_*/
