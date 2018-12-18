// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_
#define MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

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
class DLLExport PeakClusterProjection {
public:
  /// Constructor
  PeakClusterProjection(boost::shared_ptr<Mantid::API::IMDWorkspace> &mdWS);
  /// Constructor
  PeakClusterProjection(
      boost::shared_ptr<Mantid::API::IMDHistoWorkspace> &mdWS);
  /// Constructor
  PeakClusterProjection(
      boost::shared_ptr<Mantid::API::IMDEventWorkspace> &mdWS);
  PeakClusterProjection(const PeakClusterProjection &) = delete;
  PeakClusterProjection &operator=(const PeakClusterProjection &) = delete;
  /// Get the signal value at the peak center
  Mantid::signal_t
  signalAtPeakCenter(const Mantid::Geometry::IPeak &peak,
                     Mantid::API::MDNormalization normalization =
                         Mantid::API::NoNormalization) const;
  /// Get the peak center
  Mantid::Kernel::V3D peakCenter(const Mantid::Geometry::IPeak &peak) const;
  /// Destructor
  virtual ~PeakClusterProjection() = default;

private:
  /// Image
  boost::shared_ptr<Mantid::API::IMDWorkspace> m_mdWS;

  /// Peak Transform
  boost::shared_ptr<Mantid::Geometry::PeakTransform> m_peakTransform;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKCLUSTERPROJECTION_H_ */
