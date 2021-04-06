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
*/
class MANTID_GEOMETRY_DLL SampleEnvironment {
public:
  SampleEnvironment(std::string name, const Container_const_sptr &getContainer);

  /// @return The name of kit
  inline const std::string name() const { return m_name; }
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

  Geometry::BoundingBox boundingBox() const;

  bool isValid(const Kernel::V3D &point) const;
  int interceptSurfaces(Track &track) const;

  void add(const IObject_const_sptr &component);

private:
  std::string m_name;
  // Element zero is always assumed to be the can
  std::vector<IObject_const_sptr> m_components;
};

// Typedef a unique_ptr
using SampleEnvironment_uptr = std::unique_ptr<SampleEnvironment>;
// Typedef a unique_ptr to const
using SampleEnvironment_const_uptr = std::unique_ptr<const SampleEnvironment>;
} // namespace Geometry
} // namespace Mantid
