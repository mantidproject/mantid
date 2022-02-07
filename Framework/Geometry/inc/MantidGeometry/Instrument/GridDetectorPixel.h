// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

@date 2018-09-28
*/
class DLLExport GridDetectorPixel : public Detector {
  friend class GridDetector;

public:
  /// A string representation of the component type
  virtual std::string type() const override { return "GridDetectorPixel"; }

  GridDetectorPixel(const std::string &name, int id, const std::shared_ptr<IObject> &shape, IComponent *parent,
                    const GridDetector *panel, size_t col, size_t row, size_t layer);

  /// Create a cloned instance with a parameter map applied
  GridDetectorPixel *cloneParameterized(const ParameterMap *map) const override {
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
