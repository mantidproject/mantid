// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_
#define MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_

#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/System.h"
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
*/
class DLLExport RectangularDetectorPixel : public Detector {
  friend class RectangularDetector;

public:
  /// A string representation of the component type
  std::string type() const override { return "RectangularDetectorPixel"; }

  RectangularDetectorPixel(const std::string &name, int id,
                           boost::shared_ptr<IObject> shape, IComponent *parent,
                           RectangularDetector *panel, size_t row, size_t col);

  RectangularDetectorPixel();

  /// Create a cloned instance with a parameter map applied
  RectangularDetectorPixel *
  cloneParameterized(const ParameterMap *map) const override {
    return new RectangularDetectorPixel(this, map);
  }

  Kernel::V3D getRelativePos() const override;

private:
  /// RectangularDetector that is the parent of this pixel.
  RectangularDetector *m_panel;
  /// Row of the pixel in the panel (y index)
  size_t m_row;
  /// Column of the pixel in the panel (x index)
  size_t m_col;

protected:
  /// Constructor for parametrized version
  RectangularDetectorPixel(const RectangularDetectorPixel *base,
                           const ParameterMap *map);
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_RECTANGULARDETECTORPIXEL_H_ */
