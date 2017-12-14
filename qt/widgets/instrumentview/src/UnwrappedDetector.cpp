#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/Object.h"

using namespace Mantid::Geometry;

namespace MantidQt {
namespace MantidWidgets {

UnwrappedDetector::UnwrappedDetector()
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detID(0) {
  color[0] = 0;
  color[1] = 0;
  color[2] = 0;
}

UnwrappedDetector::UnwrappedDetector(const unsigned char *c,
                                     const IDetector &det)
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detID(det.getID()),
      position(det.getPos()), rotation(det.getRotation()), shape(det.shape()),
      scaleFactor(det.getScaleFactor()) {
  color[0] = *c;
  color[1] = *(c + 1);
  color[2] = *(c + 2);
}

UnwrappedDetector::UnwrappedDetector(unsigned char r, unsigned char g,
                                     unsigned char b, Mantid::detid_t detID,
                                     const Mantid::Kernel::V3D &pos,
                                     const Mantid::Kernel::Quat &rot,
                                     const Mantid::Kernel::V3D &scaleFactor,
                                     boost::shared_ptr<const Object> shape)
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0), detID(detID),
      position(pos), rotation(rot), scaleFactor(scaleFactor) {
  color[0] = r;
  color[1] = g;
  color[2] = b;

  this->shape = shape;
}

/** Copy constructor */
UnwrappedDetector::UnwrappedDetector(const UnwrappedDetector &other) {
  this->operator=(other);
}

/** Assignment operator */
UnwrappedDetector &UnwrappedDetector::
operator=(const UnwrappedDetector &other) {
  color[0] = other.color[0];
  color[1] = other.color[1];
  color[2] = other.color[2];
  u = other.u;
  v = other.v;
  width = other.width;
  height = other.height;
  uscale = other.uscale;
  vscale = other.vscale;
  detID = other.detID;
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