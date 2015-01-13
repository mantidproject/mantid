#ifndef MANTID_API_GRIDDOMAIN_H_
#define MANTID_API_GRIDDOMAIN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/FunctionDomain.h"

namespace Mantid {
namespace API {
/*Base class that represents the grid domain from which a function may take its
  arguments.
  Grids are multidimensional objects, grids are a composition of grids.

  @author Jose Borreguero
  @date Aug/28/2012

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

class MANTID_API_DLL GridDomain : public API::FunctionDomain {
public:
  GridDomain(){};
  virtual ~GridDomain(){};
  /// number of grid points
  size_t size() const;
  /// number of dimensions in the grid
  size_t nDimensions();
  /// get the grid at specified index
  boost::shared_ptr<GridDomain> getGrid(size_t index);
  /// re-scale all grids
  void reScale(const std::string &scaling);

private:
  /// composition of grids
  std::vector<boost::shared_ptr<GridDomain>> m_grids;

}; // class IGridDomain

/// typedef for a shared pointer
typedef boost::shared_ptr<GridDomain> GridDomain_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_GRIDDOMAIN_H_*/
