//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

namespace Mantid {
namespace Geometry {

IObjComponent::IObjComponent() { handle = new GeometryHandler(this); }

/** Constructor, specifying the GeometryHandler (renderer engine)
 * for this IObjComponent.
 */
IObjComponent::IObjComponent(GeometryHandler *the_handler) {
  handle = the_handler;
}

// Looking to get rid of the first of these constructors in due course (and
// probably add others)
IObjComponent::~IObjComponent() {
  if (handle != nullptr)
    delete handle;
}

/**
 * Set the geometry handler for IObjComponent
 * @param[in] h is pointer to the geometry handler. don't delete this pointer in
 * the calling function.
 */
void IObjComponent::setGeometryHandler(GeometryHandler *h) {
  if (h == nullptr)
    return;
  this->handle = h;
}

} // namespace Geometry
} // namespace Mantid
