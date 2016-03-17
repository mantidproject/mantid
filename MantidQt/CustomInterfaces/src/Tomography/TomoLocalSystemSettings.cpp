#include "MantidQtCustomInterfaces/Tomography/TomoLocalSystemSettings.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Path.h>

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoLocalSystemSettings::g_defBasePathTomoData =
#ifdef _WIN32
    "D:/data";
#else
    "~/imat-data/";
#endif

const std::string TomoLocalSystemSettings::g_defRemoteDriveOrMountPoint;

const std::string TomoLocalSystemSettings::g_defReconScriptsPath =
#ifdef _WIN32
    "C:/MantidInstall/scripts";
#else
    Poco::Path(
        Mantid::Kernel::ConfigService::Instance().getDirectoryOfExecutable())
        .parent()
        .append("scripts")
        .toString();
#endif

const std::string TomoLocalSystemSettings::g_defOutputPathCompPreProcessed =
    "pre_processed";

const std::string TomoLocalSystemSettings::g_defOutputPathCompReconst =
    "processed";

/**
 * Default initialization which should be enough in most cases.
 */
TomoLocalSystemSettings::TomoLocalSystemSettings()
    : m_basePathTomoData(g_defBasePathTomoData),
      m_remoteDriveOrMountPoint(g_defRemoteDriveOrMountPoint),
      m_reconScriptsPath(g_defReconScriptsPath),
      m_outputPathCompPreProcessed(g_defOutputPathCompPreProcessed),
      m_outputPathCompReconst(g_defOutputPathCompReconst), m_maxProcesses(4),
      m_cores(4) {}

} // namespace CustomInterfaces
} // namespace MantidQt
