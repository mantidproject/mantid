#include "MantidQtWidgets/Common/PluginLibraries.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/LibraryManager.h"
#include "MantidKernel/Strings.h"

#include <QtGlobal>

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::LibraryManager;

namespace MantidQt {
namespace API {

/**
 * Retrieve the path to some qt-related plugins from the mantid configuration
 * If the value contains %V then this sequence is replaced with the major
 * version of Qt
 * @param key The name of a key in the Mantid config
 * @return The value of the key
 */
std::string qtPluginPathFromCfg(std::string key) {
  static std::string qtMajorVersion;
  static std::string qtTag("%V");
  if (qtMajorVersion.empty()) {
    qtMajorVersion = std::string(QT_VERSION_STR, 0, 1);
  }
  using Mantid::Kernel::Strings::replace;
  return replace(ConfigService::Instance().getString(key), qtTag,
                 qtMajorVersion);
}

/**
 * Load all plugins from the path specified by the given configuration key.
 * @param key The name of a key in the Mantid config. It's value may contain
 * %V to specify the Qt version
 * @return The number of libraries successfully loaded
 */
int loadPluginsFromCfgPath(std::string key) {
  return loadPluginsFromPath(qtPluginPathFromCfg(std::move(key)));
}

/**
 * Load all plugins from the path specified.
 * @return The number of libraries successfully loaded
 */
int loadPluginsFromPath(std::string path) {
  return LibraryManager::Instance().OpenAllLibraries(path);
}
}
}
