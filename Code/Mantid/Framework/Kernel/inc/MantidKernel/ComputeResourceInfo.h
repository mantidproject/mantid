#ifndef MANTID_KERNEL_COMPUTERESOURCEINFO_H_
#define MANTID_KERNEL_COMPUTERESOURCEINFO_H_

#include <string>

#include "MantidKernel/DllConfig.h"

namespace Poco {
namespace XML {
class Element;
}
}

namespace Mantid {
namespace Kernel {

class FacilityInfo;

/**
Holds information about a compute resource present in a facility.

At the moment (remote) compute resources are defined by their name,
the URL they can be accessed at, and the type of remote job manager
that they use/require (Mantid web service API, LSF, etc.).

Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_KERNEL_DLL ComputeResourceInfo {
public:
  /// constructor - from facility info and the element for this resource
  ComputeResourceInfo(const FacilityInfo *f, const Poco::XML::Element *elem);

  /// Name of the compute resource
  std::string name() const;

  /// Base URL the compute resource
  std::string baseURL() const ;

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
MANTID_KERNEL_DLL std::ostream &
operator<<(std::ostream &buffer, const  ComputeResourceInfo &cr);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_COMPUTERESOURCEINFO_H_ */
