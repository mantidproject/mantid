// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/Container.h"

namespace Mantid {
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Geometry {
class Track;

/**
  Defines a single instance of a SampleEnvironment. It houses a single can
  along with the other components specified by a SampleEnvironmentSpec.

  Components are kept as defined, with the goniometer at zero, and never carry a goniometer
  rotation. Each one records whether it is mounted on the goniometer, so turns with the sample.
*/
class MANTID_GEOMETRY_DLL SampleEnvironment {
public:
  /// Whether a part turns with the sample unless it says otherwise: only the container, index 0, which holds it.
  static constexpr bool onGoniometerByDefault(const size_t index) { return index == 0; }

  SampleEnvironment(std::string name, const Container_const_sptr &getContainer,
                    bool containerOnGoniometer = onGoniometerByDefault(0));

  /// @return The name of kit
  inline const std::string &name() const { return m_name; }
  /// @return The name of can
  inline const std::string containerID() const { return getContainer().id(); }
  /// @return A const ptr to the can instance
  inline const Container &getContainer() const {
    if (m_components.empty())
      throw std::runtime_error("Cannot get container from empty environment");
    Container_const_sptr can = std::static_pointer_cast<const Container>(m_components.front());
    return *can;
  }
  /// @return The number of elements the environment is composed of
  inline size_t nelements() const { return m_components.size(); }

  /**
   * Returns the requested IObject. The zero-th index is considered the
   * container
   * @throws std::out_of_range
   */
  const IObject &getComponent(const size_t index) const;
  const IObject_const_sptr getComponentPtr(const size_t index) const;
  /// Whether the component turns with the sample when a goniometer is set. Index 0 is the container.
  bool isOnGoniometer(const size_t index) const { return m_onGoniometer.at(index); }

  Geometry::BoundingBox boundingBox() const;

  bool isValid(const Kernel::V3D &point) const;
  int interceptSurfaces(Track &track) const;

  void add(const IObject_const_sptr &component, bool onGoniometer = onGoniometerByDefault(1));

  /// A view of this environment as the goniometer places it: mounted components turned by
  /// goniometerR, fixed ones shared. Always built from the own-frame original, so turning a view
  /// again starts afresh rather than adding to the turn.
  std::shared_ptr<SampleEnvironment> inLabFrame(const Kernel::Matrix<double> &goniometerR) const;
  /// The environment as defined: this one, or the original a lab-frame view was made from.
  const SampleEnvironment &ownFrame() const { return m_ownFrame ? *m_ownFrame : *this; }

private:
  std::string m_name;
  // Element zero is always assumed to be the can
  std::vector<IObject_const_sptr> m_components;
  std::vector<bool> m_onGoniometer;                    ///< Parallel to m_components
  std::shared_ptr<const SampleEnvironment> m_ownFrame; ///< Set only on a view made by inLabFrame
};

// Typedef a unique_ptr
using SampleEnvironment_uptr = std::unique_ptr<SampleEnvironment>;
// Typedef a unique_ptr to const
using SampleEnvironment_const_uptr = std::unique_ptr<const SampleEnvironment>;
} // namespace Geometry
} // namespace Mantid
