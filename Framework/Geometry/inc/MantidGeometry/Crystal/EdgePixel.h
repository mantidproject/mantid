#ifndef MANTID_GEOMETRY_EDGEPIXEL_H_
#define MANTID_GEOMETRY_EDGEPIXEL_H_

#include "MantidGeometry/Instrument.h"

namespace Mantid {
namespace Geometry {

/**
  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/// Function to find peaks near detector edge
MANTID_GEOMETRY_DLL bool edgePixel(Geometry::Instrument_const_sptr inst,
                                   std::string bankName, int col, int row,
                                   int Edge);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_EDGEPIXEL_H_ */
