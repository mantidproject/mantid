// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include <memory>
#include <optional>
#include <string>

namespace Mantid {
namespace Geometry {

/** PeakShape : Abstract type to describes the shape of a peak.
 */
class MANTID_GEOMETRY_DLL PeakShape {
public:
  /// Coordinte frame used upon creation
  virtual Mantid::Kernel::SpecialCoordinateSystem frame() const = 0;
  /// Serialize
  virtual std::string toJSON() const = 0;
  /// Deep copy this
  virtual PeakShape *clone() const = 0;
  /// Algorithm
  virtual std::string algorithmName() const = 0;
  /// Algorithm Version
  virtual int algorithmVersion() const = 0;
  /// Shape name
  virtual std::string shapeName() const = 0;
  /// For selecting different radius types.
  enum RadiusType { Radius = 0, OuterRadius = 1, InnerRadius = 2 };
  /// Radius
  virtual std::optional<double> radius(RadiusType type) const = 0;
  /// Destructor
  virtual ~PeakShape() = default;
};

using PeakShape_sptr = std::shared_ptr<PeakShape>;
using PeakShape_const_sptr = std::shared_ptr<const PeakShape>;

} // namespace Geometry
} // namespace Mantid
