// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/ConcretePeakTransformFactory.h"
#include "MantidGeometry/Crystal/PeakTransform.h"

namespace Mantid {
namespace Geometry {
/**
@class PeakTransformQLab
Used to remap coordinates into a form consistent with an axis reordering.
*/
class MANTID_GEOMETRY_DLL PeakTransformQLab : public PeakTransform {
public:
  static std::string name() { return "Q (lab frame)"; }
  /// Constructor
  PeakTransformQLab();
  /// Constructor
  PeakTransformQLab(const std::string &xPlotLabel, const std::string &yPlotLabel);
  /// Virtual constructor
  PeakTransform_sptr clone() const override;
  /// Transform peak.
  Mantid::Kernel::V3D transformPeak(const Mantid::Geometry::IPeak &peak) const override;
  /// Get the transform friendly name.
  std::string getFriendlyName() const override { return name(); }
  /// Getter for the special coordinate representation of this transform type.
  Mantid::Kernel::SpecialCoordinateSystem getCoordinateSystem() const override;
};

/// Typedef a factory for type of PeaksTransform.
using PeakTransformQLabFactory = ConcretePeakTransformFactory<PeakTransformQLab>;
} // namespace Geometry
} // namespace Mantid
