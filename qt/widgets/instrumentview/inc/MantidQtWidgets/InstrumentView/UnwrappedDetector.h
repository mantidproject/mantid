#ifndef UNWRAPPEDDETECTOR_H
#define UNWRAPPEDDETECTOR_H

#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidQtWidgets/InstrumentView/GLColor.h"
#include <boost/shared_ptr.hpp>
#include <limits>

namespace Mantid {
namespace Geometry {
class IDetector;
class IObject;
} // namespace Geometry
} // namespace Mantid

namespace MantidQt {
namespace MantidWidgets {
/**
\class UnwrappedDetector
\brief Class helper for drawing detectors on unwraped surfaces
\date 15 Nov 2010
\author Roman Tolchenov, Tessella plc

This class keeps information used to draw a detector on an unwrapped surface.

*/
class UnwrappedDetector {
public:
  UnwrappedDetector();
  UnwrappedDetector(GLColor color, size_t detIndex);
  UnwrappedDetector(const UnwrappedDetector &other);
  UnwrappedDetector &operator=(const UnwrappedDetector &other);
  bool empty() const;
  GLColor color; ///< red, green, blue colour components (0 - 255)
  double u;      ///< horizontal "unwrapped" coordinate
  double v;      ///< vertical "unwrapped" coordinate
  double width;  ///< detector width in units of u
  double height; ///< detector height in units of v
  double uscale; ///< scaling factor in u direction
  double vscale; ///< scaling factor in v direction
  size_t detIndex = std::numeric_limits<size_t>::max(); ///< Detector Index in
  ///< ComponentInfo/DetectorInfo.
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif