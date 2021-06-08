// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDWorkspace.h"
#include "MantidCrystal/DllConfig.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include <memory>

namespace Mantid {
namespace Geometry {
class IPeak;
class PeakTransform;
} // namespace Geometry
namespace API {
class IMDHistoWorkspace;
class IMDEventWorkspace;
} // namespace API
namespace Crystal {

/** PeakClusterProjection : Maps peaks onto IMDHistoWorkspaces and returns the
  signal value at the peak center.
*/
class MANTID_CRYSTAL_DLL PeakClusterProjection {
public:
  /// Constructor
  PeakClusterProjection(std::shared_ptr<Mantid::API::IMDWorkspace> &mdWS);
  /// Constructor
  PeakClusterProjection(std::shared_ptr<Mantid::API::IMDHistoWorkspace> &mdWS);
  /// Constructor
  PeakClusterProjection(std::shared_ptr<Mantid::API::IMDEventWorkspace> &mdWS);
  PeakClusterProjection(const PeakClusterProjection &) = delete;
  PeakClusterProjection &operator=(const PeakClusterProjection &) = delete;
  /// Get the signal value at the peak center
  Mantid::signal_t signalAtPeakCenter(const Mantid::Geometry::IPeak &peak,
                                      Mantid::API::MDNormalization normalization = Mantid::API::NoNormalization) const;
  /// Get the peak center
  Mantid::Kernel::V3D peakCenter(const Mantid::Geometry::IPeak &peak) const;
  /// Destructor
  virtual ~PeakClusterProjection() = default;

private:
  /// Image
  std::shared_ptr<Mantid::API::IMDWorkspace> m_mdWS;

  /// Peak Transform
  std::shared_ptr<Mantid::Geometry::PeakTransform> m_peakTransform;
};

} // namespace Crystal
} // namespace Mantid
