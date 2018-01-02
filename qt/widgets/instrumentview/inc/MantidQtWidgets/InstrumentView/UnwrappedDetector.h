#ifndef UNWRAPPEDDETECTOR_H
#define UNWRAPPEDDETECTOR_H

#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/shared_ptr.hpp>
#include "MantidQtWidgets/InstrumentView/GLColor.h"

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
  UnwrappedDetector(GLColor color, Mantid::detid_t detID, size_t detIndex,
                    const Mantid::Kernel::V3D &pos,
                    const Mantid::Kernel::Quat &rot,
                    const Mantid::Kernel::V3D &scaleFactor,
                    boost::shared_ptr<const Mantid::Geometry::IObject> shape);
  UnwrappedDetector(const UnwrappedDetector &other);
  UnwrappedDetector &operator=(const UnwrappedDetector &other);
  bool isValid() const;
  GLColor color; ///< red, green, blue colour components (0 - 255)
  double u;               ///< horizontal "unwrapped" coordinate
  double v;               ///< vertical "unwrapped" coordinate
  double width;           ///< detector width in units of u
  double height;          ///< detector height in units of v
  double uscale;          ///< scaling factor in u direction
  double vscale;          ///< scaling factor in v direction
  Mantid::detid_t detID;  ///< Detector ID
  size_t detIndex; ///< Detector Index in ComponentInfo/DetectorInfo.
  Mantid::Kernel::V3D position;  ///< Detector position
  Mantid::Kernel::Quat rotation; ///< Detector orientation
  boost::shared_ptr<const Mantid::Geometry::IObject>
      shape;                       ///< Shape of the detector
  Mantid::Kernel::V3D scaleFactor; ///< Detector's scale factor
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif