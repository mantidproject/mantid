#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/CSGObject.h"

using namespace Mantid::Geometry;

namespace MantidQt {
namespace MantidWidgets {

UnwrappedDetector::UnwrappedDetector()
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detID(0),
      detIndex(0) {
  color = GLColor(0, 0, 0);
}

UnwrappedDetector::UnwrappedDetector(GLColor color, Mantid::detid_t detID,
                                     size_t detIndex,
                                     const Mantid::Kernel::V3D &pos,
                                     const Mantid::Kernel::Quat &rot,
                                     const Mantid::Kernel::V3D &scaleFactor,
                                     boost::shared_ptr<const IObject> shape)
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detID(detID),
      detIndex(detIndex), position(pos), rotation(rot),
      scaleFactor(scaleFactor) {
  this->color = color;
  this->shape = shape;
}

/** Copy constructor */
UnwrappedDetector::UnwrappedDetector(const UnwrappedDetector &other) {
  this->operator=(other);
}

/** Assignment operator */
UnwrappedDetector &UnwrappedDetector::
operator=(const UnwrappedDetector &other) {
  color = other.color;
  u = other.u;
  v = other.v;
  width = other.width;
  height = other.height;
  uscale = other.uscale;
  vscale = other.vscale;
  detID = other.detID;
  detIndex = other.detIndex;
  position = other.position;
  rotation = other.rotation;
  shape = other.shape;
  scaleFactor = other.scaleFactor;
  return *this;
}

/** Check if the object is valid*/
bool UnwrappedDetector::isValid() const { return static_cast<bool>(shape); }
} // namespace MantidWidgets
} // namespace MantidQt