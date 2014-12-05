#ifndef MANTID_API_LINEARSCALE_H_
#define MANTID_API_LINEARSCALE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITransformScale.h"


namespace Mantid
{
namespace API
{
/*Base class  representing a linear scaling transformation acting on a one-dimensional grid domain

  @author Jose Borreguero
  @date Aug/28/2012

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>.
*/

class MANTID_API_DLL LinearScale : public API::ITransformScale
{
public:
  LinearScale() {};
  virtual ~LinearScale() {};
  /// The scaling transformation. First and last elements of the grid remain unchanged
  virtual const std::string name() const { return "LinearScale"; }
  virtual void transform( std::vector<double> &gd );
}; // class LinearScale


} // namespace API
} // namespace Mantid

#endif /*MANTID_API_LINEARSCALE_H_*/
