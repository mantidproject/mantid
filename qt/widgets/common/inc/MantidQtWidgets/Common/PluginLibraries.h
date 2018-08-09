#ifndef MANTIDQT_WIDGETS_COMMON_PLUGINLIBRARIES_H
#define MANTIDQT_WIDGETS_COMMON_PLUGINLIBRARIES_H
/**
  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

#include "MantidQtWidgets/Common/DllOption.h"

#include <string>

namespace MantidQt {
namespace API {

EXPORT_OPT_MANTIDQT_COMMON std::string qtPluginPathFromCfg(std::string key);

/// Load plugins from a path given by the key in the config service
EXPORT_OPT_MANTIDQT_COMMON int loadPluginsFromCfgPath(std::string key);

/// Load plugins from a path
EXPORT_OPT_MANTIDQT_COMMON int loadPluginsFromPath(std::string path);
} // namespace API
} // namespace MantidQt

#endif // PLUGINLIBRARYLOADER_H
