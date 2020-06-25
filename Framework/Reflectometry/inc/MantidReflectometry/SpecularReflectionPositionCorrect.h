// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidReflectometry/DllConfig.h"
#include "MantidReflectometry/SpecularReflectionAlgorithm.h"

namespace Mantid {
namespace Reflectometry {

/** SpecularReflectionPositionCorrect : Algorithm to perform vertical position
 corrections based on the specular reflection condition.
 */
class MANTID_REFLECTOMETRY_DLL SpecularReflectionPositionCorrect
    : public SpecularReflectionAlgorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Correct detector positions vertically based on the specular "
           "reflection condition.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  /// Correct detector positions.
  void correctPosition(const API::MatrixWorkspace_sptr &toCorrect,
                       const double &twoThetaInDeg,
                       const Geometry::IComponent_const_sptr &sample,
                       const Geometry::IComponent_const_sptr &detector);

  /// Move detectors.
  void moveDetectors(const API::MatrixWorkspace_sptr &toCorrect,
                     Geometry::IComponent_const_sptr detector,
                     const Geometry::IComponent_const_sptr &sample,
                     const double &upOffset, const double &acrossOffset,
                     const Mantid::Kernel::V3D &detectorPosition);
};

} // namespace Reflectometry
} // namespace Mantid
