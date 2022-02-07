// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/DllOption.h"

#include <string>

namespace MantidQt {
namespace API {

EXPORT_OPT_MANTIDQT_COMMON std::string qtPluginPathFromCfg(const std::string &key);

/// Load plugins from a path given by the key in the config service
EXPORT_OPT_MANTIDQT_COMMON int loadPluginsFromCfgPath(const std::string &key);

/// Load plugins from a path
EXPORT_OPT_MANTIDQT_COMMON int loadPluginsFromPath(const std::string &path);
} // namespace API
} // namespace MantidQt
