#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONTOOLSUSERSETTINGS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONTOOLSUSERSETTINGS_H_

#include "MantidQtCustomInterfaces/Tomography/ToolConfigAstraToolbox.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigCustom.h"
#include "MantidQtCustomInterfaces/Tomography/ToolConfigTomoPy.h"

#include <string>

namespace MantidQt {
namespace CustomInterfaces {

/**
Settings for a set of tomographic reconstruction tools supported.

Copyright &copy; 2014,205 ISIS Rutherford Appleton Laboratory, NScD
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
struct TomoReconToolsUserSettings {
  // This is just too basic at the moment. We probably want to store
  // here the real settings objects for all the tools, and rather than
  // this horror have a dictionary of tools-settings or
  // similar. Waiting to see what happens with Savu and others.
  MantidQt::CustomInterfaces::ToolConfigTomoPy tomoPy;
  MantidQt::CustomInterfaces::ToolConfigAstraToolbox astra;
  std::string CCPi;
  std::string savu;
  MantidQt::CustomInterfaces::ToolConfigCustom custom;
};

} // namespace CustomInterfaces
} // namespace MantidQt

#endif // MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMORECONTOOLSUSERSETTINGS_H_
