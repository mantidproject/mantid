#include "MantidQtWidgets/InstrumentView/UnwrappedDetector.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Objects/CSGObject.h"

using namespace Mantid::Geometry;

namespace MantidQt {
namespace MantidWidgets {

UnwrappedDetector::UnwrappedDetector()
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0) {
  color = GLColor(0, 0, 0);
}

UnwrappedDetector::UnwrappedDetector(GLColor color, size_t detIndex)
    : u(0), v(0), width(0), height(0), uscale(0), vscale(0),
      detIndex(detIndex) {
  this->color = color;
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
  detIndex = other.detIndex;
  return *this;
}

bool UnwrappedDetector::empty() const {
  return detIndex == std::numeric_limits<size_t>::max();
}

} // namespace MantidWidgets
} // namespace MantidQt