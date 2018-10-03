// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid {
namespace Geometry {
using Geometry::BoundingBox;
using Geometry::Track;
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
 * Generate a random point within one of the environment's components. The
 * method first selects a random component and then selects a random point
 * within that component using Object::generatePointObject
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The generated point
 */
Kernel::V3D
SampleEnvironment::generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                                 const size_t maxAttempts) const {
  auto componentIndex = rng.nextInt(1, static_cast<int>(nelements())) - 1;
  return m_components[componentIndex]->generatePointInObject(rng, maxAttempts);
}

/**
 * Generate a random point within one of the environment's components. The
 * method first selects a random component and then selects a random point
 * within that component using Object::generatePointObject
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @param activeRegion Restrict the generated point to be defined by this box
 * @param maxAttempts The maximum number of attempts at generating a point
 * @return The generated point
 */
Kernel::V3D
SampleEnvironment::generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                                 const Geometry::BoundingBox &activeRegion,
                                 const size_t maxAttempts) const {
  auto componentIndex = rng.nextInt(1, static_cast<int>(nelements())) - 1;
  return m_components[componentIndex]->generatePointInObject(rng, activeRegion,
                                                             maxAttempts);
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
void SampleEnvironment::add(const IObject_const_sptr &component) {
  m_components.emplace_back(component);
}
} // namespace Geometry
} // namespace Mantid
