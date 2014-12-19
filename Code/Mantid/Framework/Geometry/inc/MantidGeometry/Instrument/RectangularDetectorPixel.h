#ifndef MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_
#define MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_

#include "MantidKernel/System.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

// Forward declaration
class RectangularDetector;

/** RectangularDetectorPixel: a sub-class of Detector
  that is one pixel inside a RectangularDetector.

  The position of the pixel is calculated on the fly from the row/column
  of the pixel and the size of the parent (which is parametrized).

  @date 2011-11-22

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
class DLLExport RectangularDetectorPixel : public Detector {
  friend class RectangularDetector;

public:
  /// A string representation of the component type
  virtual std::string type() const { return "RectangularDetectorPixel"; }

  /// Constructor for parametrized version
  RectangularDetectorPixel(const RectangularDetectorPixel *base,
                           const ParameterMap *map);
  RectangularDetectorPixel(const std::string &name, int it,
                           boost::shared_ptr<Object> shape, IComponent *parent,
                           RectangularDetector *panel, size_t row, size_t col);

  RectangularDetectorPixel();
  virtual ~RectangularDetectorPixel();

  virtual const Kernel::V3D getRelativePos() const;

protected:
  /// RectangularDetector that is the parent of this pixel.
  RectangularDetector *m_panel;
  /// Row of the pixel in the panel (y index)
  size_t m_row;
  /// Column of the pixel in the panel (x index)
  size_t m_col;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_ */
