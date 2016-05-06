#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOSYSTEMSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOSYSTEMSETTINGS_H_

#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettingsLocal.h"
#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettingsRemote.h"

#include "MantidQtCustomInterfaces/DllConfig.h"

#include <string>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings for the paths on the local and remote machines, path
components, and related parameters. Most of these options could be
hardcoded but it is convenient to be able to manipulate them, for
flexibility and for testing purposes. Consists of "local" and "remote"
parameters as well as other parameters that are global.

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
struct MANTIDQT_CUSTOMINTERFACES_DLL TomoSystemSettings {
  // TODO use dictionary(es) for the string settings when they settle down
  TomoSystemSettingsLocal m_local;
  TomoSystemSettingsRemote m_remote;

  std::vector<std::string> m_pathComponents;

  std::string m_samplesDirPrefix;
  std::string m_flatsDirPrefix;
  std::string m_darksDirPrefix;

  /// path component for the pre-processed file (outputs)
  std::string m_outputPathCompPreProcessed;
  /// path component for the reconstructed files (outputs)
  std::string m_outputPathCompReconst;

  static const std::string g_defSamplesDirPrefix;
  static const std::string g_defFlatsDirPrefix;
  static const std::string g_defDarksDirPrefix;

  static const std::string g_defOutputPathCompPreProcessed;
  static const std::string g_defOutputPathCompReconst;

  TomoSystemSettings();
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOSYSTEMSETTINGS_H_
