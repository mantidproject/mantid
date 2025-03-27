// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidKernel/V3D.h"
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {

class IObject;
struct MANTID_GEOMETRY_DLL BeamProfile {
public:
  static std::optional<BeamProfile> create(const IComponent_const_sptr source, const Kernel::V3D beamDirection) {
    try {
      return BeamProfile(source, beamDirection);
    } catch (const std::exception &e) {
      return std::nullopt;
    }
  }

  BeamProfile() = default;
  BeamProfile(const BeamProfile &) = default;

  std::string shape;
  double radius;
  double height;
  double width;
  Kernel::V3D direction;
  Kernel::V3D center;

private:
  BeamProfile(const IComponent_const_sptr source, const Kernel::V3D beamDirection);
};

namespace GaugeVolume {

MANTID_GEOMETRY_DLL std::shared_ptr<IObject> determineGaugeVolume(const IObject &sample,
                                                                  const BeamProfile &beamProfile);

} // namespace GaugeVolume
} // namespace Geometry
} // namespace Mantid
