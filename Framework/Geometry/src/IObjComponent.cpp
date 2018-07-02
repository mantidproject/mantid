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

/**
 * Copy constructor
 */
IObjComponent::IObjComponent(const IObjComponent &) {
  // Copy constructor just creates new handle. Copies nothing.
  handle = new GeometryHandler(this);
}

/**
 * Assignment operator
 * @param rhs The rvalue to copy into this object
 * @returns A reference to this object
 */
IObjComponent &IObjComponent::operator=(const IObjComponent &rhs) {
  if (&rhs != this) {
    // Assignment operator copies nothing. Just creates new handle.
    handle = new GeometryHandler(this);
  }
  return *this;
}
} // namespace Geometry
} // namespace Mantid
