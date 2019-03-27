// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"

namespace Mantid {
namespace Geometry {

IObjComponent::IObjComponent() {
  handle = std::make_unique<GeometryHandler>(this);
}

/** Constructor, specifying the GeometryHandler (renderer engine)
 * for this IObjComponent.
 */
IObjComponent::IObjComponent(GeometryHandler *the_handler) {
  handle.reset(the_handler);
}

// Looking to get rid of the first of these constructors in due course (and
// probably add others)
IObjComponent::~IObjComponent() = default;

/**
 * Set the geometry handler for IObjComponent
 * @param[in] h is pointer to the geometry handler. don't delete this pointer in
 * the calling function.
 */
void IObjComponent::setGeometryHandler(GeometryHandler *h) {
  if (h == nullptr)
    return;
  this->handle.reset(h);
}

/**
 * Copy constructor
 */
IObjComponent::IObjComponent(const IObjComponent & /*unused*/) {
  // Copy constructor just creates new handle. Copies nothing.
  handle = std::make_unique<GeometryHandler>(this);
}

/**
 * Assignment operator
 * @param rhs The rvalue to copy into this object
 * @returns A reference to this object
 */
IObjComponent &IObjComponent::operator=(const IObjComponent &rhs) {
  if (&rhs != this) {
    // Assignment operator copies nothing. Just creates new handle.
    handle = std::make_unique<GeometryHandler>(this);
  }
  return *this;
}
} // namespace Geometry
} // namespace Mantid
