#include "MantidQtCustomInterfaces/Tomography/TomoRemoteSystemSettings.h"

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoRemoteSystemSettings::g_defBasePathTomoData =
    "/work/imat";

const std::string TomoRemoteSystemSettings::g_defBasePathReconScripts =
    "/work/imat/phase_commissioning";

/**
 * Default initialization which should be enough in most cases.
 */
TomoRemoteSystemSettings::TomoRemoteSystemSettings()
    : m_basePathTomoData(g_defBasePathTomoData),
      m_basePathReconScripts(g_defBasePathReconScripts), m_nodes(2),
      m_cores(4) {}

} // namespace CustomInterfaces
} // namespace MantidQt
