//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Track.h"

namespace Mantid {
namespace Geometry {
using Geometry::BoundingBox;
using Geometry::Track;
using Kernel::Material;
using Kernel::V3D;

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------

/**
 * Constructor specifying a name for the environment. It is empty by default and
 * required by various other users of it
 * @param name A human-readable name for the kit
 * @param container The object that represents the can
 */
SampleEnvironment::SampleEnvironment(std::string name,
                                     Container_const_sptr container)
    : m_name(std::move(name)), m_components(1, container) {}

/**
 * @return An axis-aligned BoundingBox object that encompasses the whole kit.
 */
Geometry::BoundingBox SampleEnvironment::boundingBox() const {
  BoundingBox box;
  for (const auto &component : m_components) {
    box.grow(component->getBoundingBox());
  }
  return box;
}

/**
 * Is the point given a valid point within the environment
 * @param point Is the point valid within the environment
 * @returns True if the point is within the environment
 */
bool SampleEnvironment::isValid(const V3D &point) const {
  for (const auto &component : m_components) {
    if (component->isValid(point))
      return true;
  }
  return false;
}

/**
 * Update the given track with intersections within the environment
 * @param track The track is updated with an intersection with the
 *        environment
 * @return The total number of segments added to the track
 */
int SampleEnvironment::interceptSurfaces(Track &track) const {
  int nsegments(0);
  for (const auto &component : m_components) {
    nsegments += component->interceptSurface(track);
  }
  return nsegments;
}

/**
 * @param component An object defining some component of the environment
 */
void SampleEnvironment::add(const Object_const_sptr &component) {
  m_components.emplace_back(component);
}
}
}
