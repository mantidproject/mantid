#ifndef MANTID_GEOMETRY_GRIDDETECTORPIXEL_H_
#define MANTID_GEOMETRY_GRIDDETECTORPIXEL_H_

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Geometry {

// Forward declaration
class GridDetector;

/** GridrDetectorPixel: a sub-class of Detector
that is one pixel inside a GridDetector.

The position of the pixel is calculated on the fly from the row/column/plane
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
class DLLExport GridDetectorPixel : public Detector {
  friend class GridDetector;

public:
  /// A string representation of the component type
  virtual std::string type() const override { return "GridDetectorPixel"; }

  GridDetectorPixel(const std::string &name, int id,
                    boost::shared_ptr<IObject> shape, IComponent *parent,
                    const GridDetector *panel, size_t col, size_t row,
                    size_t layer);

  /// Create a cloned instance with a parameter map applied
  GridDetectorPixel *
  cloneParameterized(const ParameterMap *map) const override {
    return new GridDetectorPixel(this, map);
  }

  Kernel::V3D getRelativePos() const override;

private:
  /// GridDetector that is the parent of this pixel.
  const GridDetector *m_panel;
  /// Row of the pixel in the panel (x/col index)
  size_t m_col;
  /// Column of the pixel in the panel (y/row index)
  size_t m_row;
  /// Plane of the pixel in the panel (z/layer index)
  size_t m_layer;

protected:
  /// Constructor for parametrized version
  GridDetectorPixel(const GridDetectorPixel *base, const ParameterMap *map);
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_GRIDDETECTORPIXEL_H_ */
