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

const IObject &SampleEnvironment::getComponent(const size_t index) const {
  if (index > this->nelements()) {
    std::stringstream msg;
    msg << "Requested SampleEnvironment element that is out of range: " << index
        << " < " << this->nelements();
    throw std::out_of_range(msg.str());
  }
  return *(m_components[index]);
}

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
  return std::any_of(
      m_components.cbegin(), m_components.cend(),
      [&point](const auto &component) { return component->isValid(point); });
}

/**
 * Update the given track with intersections within the environment
 * @param track The track is updated with an intersection with the
 *        environment
 * @return The total number of segments added to the track
 */
int SampleEnvironment::interceptSurfaces(Track &track) const {
  return std::accumulate(m_components.cbegin(), m_components.cend(), 0,
                         [&track](int sum, const auto &component) {
                           return sum + component->interceptSurface(track);
                         });
}

/**
 * @param component An object defining some component of the environment
 */
void SampleEnvironment::add(const IObject_const_sptr &component) {
  m_components.emplace_back(component);
}
} // namespace Geometry
} // namespace Mantid
