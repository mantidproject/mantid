// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DataProcessorAlgorithm.h"
#include "MantidReflectometry/DllConfig.h"

#include <vector>

namespace Mantid {
namespace Geometry {
class ComponentInfo;
}
namespace Kernel {
class V3D;
}
namespace Reflectometry {

/** SpecularReflectionAlgorithm : Algorithm base class implementing generic
 methods required for specular reflection calculations.
 *
 */
class MANTID_REFLECTOMETRY_DLL SpecularReflectionAlgorithm : public Mantid::API::DataProcessorAlgorithm {
protected:
  /// Constructor
  SpecularReflectionAlgorithm() = default;

  /// Get the component index of the surface sample component
  size_t getSurfaceSampleComponentIndex(Geometry::ComponentInfo const &componentInfo) const;

  /// Get the component indices making up the detector
  std::vector<size_t> getDetectorComponentIndices(API::MatrixWorkspace const &workspace,
                                                  bool const isPointDetector) const;

  /// Get the mean position of a set of components
  static Kernel::V3D meanPosition(Geometry::ComponentInfo const &componentInfo,
                                  std::vector<size_t> const &componentIndices);

  /// Does the property have a default value.
  bool isPropertyDefault(const std::string &propertyName) const;

  /// initialize common properties
  void initCommonProperties();

  /// Calculate the twoTheta angle w.r.t the direct beam
  double calculateTwoTheta() const;
};

} // namespace Reflectometry
} // namespace Mantid
