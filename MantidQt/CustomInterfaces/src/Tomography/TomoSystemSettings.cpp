#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

std::string g_defSamplesDirPrefix = "data";
std::string g_defFlatsDirPrefix = "flat";
std::string g_defDarksDirPrefix = "dark";

/**
 * Default initialization which should be enough in most cases.
 */
TomoSystemSettings::TomoSystemSettings()
    : m_samplesDirPrefix(g_defSamplesDirPrefix),
      m_flatsDirPrefix(g_defFlatsDirPrefix),
      m_darksDirPrefix(g_defDarksDirsPrefix) {
  m_pathComponents = { "data", "RB number", "experiment name", "input type"};
}

} // namespace CustomInterfaces
} // namespace MantidQt
