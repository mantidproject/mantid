#ifndef MANTID_GEOMETRY_COMPONENTVISITORHELPER_H_
#define MANTID_GEOMETRY_COMPONENTVISITORHELPER_H_

#include "MantidGeometry/DllConfig.h"
#include <string>

namespace Mantid {
namespace Geometry {

/** ComponentVisitorHelper namespace : Helper functions for Component Visitors.
  These mainly relate to helping with common code for IDF compatibility
  problems.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace ComponentVisitorHelper {

bool MANTID_GEOMETRY_DLL matchesPackOfTubes(const std::string &nameHint);

bool MANTID_GEOMETRY_DLL matchesPSDTube(const std::string &nameHint);

} // namespace ComponentVisitorHelper
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_COMPONENTVISITORHELPER_H_ */
