#ifndef MANTID_GEOMETRY_LASZLOINTERSECTION_H_
#define MANTID_GEOMETRY_LASZLOINTERSECTION_H_

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include <stdexcept>

namespace Mantid {
namespace Geometry {
/**
This header defines an implementation of the convex polygon intersection method
by Michael Laszlo

@author Martyn Gigg
@date 2011-07-12

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

class MANTID_GEOMETRY_DLL NoIntersectionException : public std::runtime_error {
public:
  NoIntersectionException()
      : std::runtime_error(
            "No intersections found that form a complete convex polygon.") {}
};

/// Compute the intersection of two ConvexPolygons
MANTID_GEOMETRY_DLL
ConvexPolygon intersectionByLaszlo(const ConvexPolygon &P,
                                   const ConvexPolygon &Q);

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_LASZLOINTERSECTION_H_ */
