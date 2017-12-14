#ifndef UNWRAPPEDDETECTOR_H
#define UNWRAPPEDDETECTOR_H

#include "MantidGeometry/IDTypes.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include <boost/shared_ptr.hpp>

namespace Mantid {
namespace Geometry {
class IDetector;
class Object;
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
  UnwrappedDetector(const unsigned char *c,
                    const Mantid::Geometry::IDetector &det);
  UnwrappedDetector(unsigned char r, unsigned char g, unsigned char b,
                    Mantid::detid_t detID, const Mantid::Kernel::V3D &pos,
                    const Mantid::Kernel::Quat &rot,
                    const Mantid::Kernel::V3D &scaleFactor,
                    boost::shared_ptr<const Mantid::Geometry::Object> shape);
  UnwrappedDetector(const UnwrappedDetector &other);
  UnwrappedDetector &operator=(const UnwrappedDetector &other);
  bool isValid() const;
  unsigned char color[3]; ///< red, green, blue colour components (0 - 255)
  double u;               ///< horizontal "unwrapped" coordinate
  double v;               ///< vertical "unwrapped" coordinate
  double width;           ///< detector width in units of u
  double height;          ///< detector height in units of v
  double uscale;          ///< scaling factor in u direction
  double vscale;          ///< scaling factor in v direction
  Mantid::detid_t detID;  ///< Detector ID
  Mantid::Kernel::V3D position;  ///< Detector position
  Mantid::Kernel::Quat rotation; ///< Detector orientation
  boost::shared_ptr<const Mantid::Geometry::Object>
      shape;                       ///< Shape of the detector
  Mantid::Kernel::V3D scaleFactor; ///< Detector's scale factor
};
} // namespace MantidWidgets
} // namespace MantidQt
#endif