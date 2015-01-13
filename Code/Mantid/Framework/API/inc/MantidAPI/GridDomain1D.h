#ifndef MANTID_API_GRIDDOMAIN1D_H_
#define MANTID_API_GRIDDOMAIN1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <stdexcept>

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/GridDomain.h"

namespace Mantid {
namespace API {
/*Base class that represents a one dimensional grid domain,
  from which a function may take its arguments.

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

class MANTID_API_DLL GridDomain1D : public API::GridDomain {
public:
  GridDomain1D(){};
  virtual ~GridDomain1D(){};
  /// initialize
  void initialize(double &startX, double &endX, size_t &n, std::string scaling);
  /// number of grid point	s
  size_t size() const { return m_points.size(); }
  /// number of dimensions in the grid
  size_t nDimensions() { return 1; }
  void reScale(const std::string &scaling);
  void SetScalingName(const std::string scaling);
  std::vector<double> &getPoints() { return m_points; }

private:
  std::string m_scaling;
  std::vector<double> m_points;

}; // class IGridDomain

/// typedef for a shared pointer
typedef boost::shared_ptr<GridDomain1D> GridDomain1D_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_GRIDDOMAIN1D_H_*/
