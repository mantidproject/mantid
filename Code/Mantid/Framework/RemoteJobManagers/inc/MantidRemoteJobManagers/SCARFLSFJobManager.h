#ifndef MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H
#define MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H

#include "MantidKernel/DllConfig.h"
#include "MantidRemoteJobManagers/LSFJobManager.h"

namespace Mantid {
namespace RemoteJobManagers {
/**
SCARFLSFJobManager implements a remote job manager that knows how to
talk to Platform LSF web service at the SCARF cluster. This is in
principle a generic Platform LSF web service, except for the
authentication mechanism.

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
class DLLExport SCARFLSFJobManager
    : public Mantid::RemoteJobManagers::LSFJobManager {
public:
  void authenticate(std::string &username, std::string &password);

  void logout(std::string &username = std::string());
};

} // namespace RemoteJobManagers
} // namespace Mantid

#endif // MANTID_REMOTEJOBMANAGERS_SCARFLSFJOBMANAGER_H
