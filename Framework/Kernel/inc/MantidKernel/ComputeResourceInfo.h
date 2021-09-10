// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <iosfwd>
#include <string>

namespace Poco {
namespace XML {
class Element;
}
} // namespace Poco

namespace Mantid {
namespace Kernel {

class FacilityInfo;

/**
ComputeResourceInfo holds information about / represents a compute
resource present in a facility.

At the moment (remote) compute resources are defined by their name,
the URL they can be accessed at, and the type of remote job manager
that they use/require (Mantid web service API, LSF, etc.).
*/
class MANTID_KERNEL_DLL ComputeResourceInfo {
public:
  /// constructor - from facility info and the element for this resource
  ComputeResourceInfo(const FacilityInfo *fac, const Poco::XML::Element *elem);

  /// Equality operator
  bool operator==(const ComputeResourceInfo &rhs) const;

  /// Name of the compute resource
  std::string name() const;

  /// Base URL the compute resource
  std::string baseURL() const;

  /// Type/class of remote job manager required to handle this resource
  std::string remoteJobManagerType() const;

  /// The facility where this compute resource is avalable
  const FacilityInfo &facility() const;

private:
  const FacilityInfo *m_facility; ///< Facility
  std::string m_name;             ///< Cluster/resource name
  std::string m_baseURL;          ///< access URL (first authentication, etc.)
  std::string m_managerType;      ///< specific remote job manager class
};

/// output to stream operator for compute resource info objects
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &buffer, const ComputeResourceInfo &cr);

} // namespace Kernel
} // namespace Mantid
