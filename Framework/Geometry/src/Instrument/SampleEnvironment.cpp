// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/ShapeRotation.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid::Geometry {
using Geometry::BoundingBox;
using Geometry::Track;
using Kernel::V3D;

namespace {
void requireOwnFrame(const IObject &component) {
  if (component.getAppliedRotation() != Kernel::Matrix<double>(3, 3, true)) {
    throw std::invalid_argument("Sample environment component '" + component.id() +
                                "' carries a goniometer rotation. Environments are kept as defined, with the "
                                "goniometer at zero, and turned only when used. Use rotate-all to orient it.");
  }
}
} // namespace

//------------------------------------------------------------------------------
// Public methods
//------------------------------------------------------------------------------

/**
 * Constructor specifying a name for the environment. It is empty by default and
 * required by various other users of it
 * @param name A human-readable name for the kit
 * @param container The object that represents the can
 * @param containerOnGoniometer Whether the can turns with the sample when a goniometer is set
 */
SampleEnvironment::SampleEnvironment(std::string name, const Container_const_sptr &container,
                                     const bool containerOnGoniometer)
    : m_name(std::move(name)), m_components(1, container), m_onGoniometer(1, containerOnGoniometer) {
  requireOwnFrame(*container);
}

const IObject &SampleEnvironment::getComponent(const size_t index) const {
  if (index > this->nelements()) {
    std::stringstream msg;
    msg << "Requested SampleEnvironment element that is out of range: " << index << " < " << this->nelements();
    throw std::out_of_range(msg.str());
  }
  return *(m_components[index]);
}

const IObject_const_sptr SampleEnvironment::getComponentPtr(const size_t index) const {
  if (index > this->nelements()) {
    std::stringstream msg;
    msg << "Requested SampleEnvironment element that is out of range: " << index << " < " << this->nelements();
    throw std::out_of_range(msg.str());
  }
  return m_components[index];
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
  return std::any_of(m_components.cbegin(), m_components.cend(),
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
                         [&track](int sum, const auto &component) { return sum + component->interceptSurface(track); });
}

/**
 * @param component An object defining some component of the environment
 * @param onGoniometer Whether it turns with the sample when a goniometer is set
 */
void SampleEnvironment::add(const IObject_const_sptr &component, const bool onGoniometer) {
  if (m_ownFrame)
    throw std::logic_error("A lab-frame environment is a view; add to the environment it was made from.");
  requireOwnFrame(*component);
  m_components.emplace_back(component);
  m_onGoniometer.emplace_back(onGoniometer);
}

std::shared_ptr<SampleEnvironment> SampleEnvironment::inLabFrame(const Kernel::Matrix<double> &goniometerR) const {
  const auto &own = ownFrame();
  // Copying shares every component, so fixed ones keep their addresses: Discus finds them by pointer.
  auto lab = std::make_shared<SampleEnvironment>(own);
  lab->m_ownFrame = m_ownFrame ? m_ownFrame : std::make_shared<const SampleEnvironment>(own);
  for (size_t i = 0; i < own.nelements(); ++i) {
    if (!own.m_onGoniometer[i])
      continue;
    if (i == 0) {
      const auto &can = own.getContainer();
      lab->m_components[i] = can.withShape(getLabFrameShape(can.getShape(), goniometerR));
    } else {
      lab->m_components[i] = getLabFrameShape(*own.m_components[i], goniometerR);
    }
  }
  return lab;
}
} // namespace Mantid::Geometry
