#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOLOCALSYSTEMSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOLOCALSYSTEMSETTINGS_H_

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings for the paths and other parameters on the local machine. Most
of these options could be hardcoded but it is convenient to be able to
manipulate them, for flexibility and for testing/commissioning.

Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
struct MANTIDQT_CUSTOMINTERFACES_DLL TomoLocalSystemSettings {
  std::string m_basePath;
  std::string m_remoteDriveOrMountPoint;
  std::string m_reconScriptsPath;
  int m_maxProcesses;
  int m_cores;

  std::string g_defBasePath;
  std::string g_defRemoteDriveOrMountPoint;
  std::string g_defReconScriptsPath;

  TomoLocalSystemsettings();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOLOCALSYSTEMSETTINGS_H_
