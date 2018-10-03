// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_COMMON_PLUGINLIBRARIES_H
#define MANTIDQT_WIDGETS_COMMON_PLUGINLIBRARIES_H


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
