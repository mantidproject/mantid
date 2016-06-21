#include "MantidQtCustomInterfaces/Tomography/TomoSystemSettingsLocal.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Path.h>

namespace MantidQt {
namespace CustomInterfaces {

const std::string TomoSystemSettingsLocal::g_defBasePathTomoData =
#ifdef _WIN32
    "D:\\";
#else
    "${HOME}/imat-data";
#endif

const std::string TomoSystemSettingsLocal::g_defRemoteDriveOrMountPoint =
#ifdef _WIN32
    "S:\\imat";
#else
    "/media/scarf";
#endif

const std::string TomoSystemSettingsLocal::g_defReconScriptsPath =
#ifdef _WIN32
    "C:/MantidInstall/scripts";
#else
    Poco::Path(
        Mantid::Kernel::ConfigService::Instance().getDirectoryOfExecutable())
        .parent()
        .append("scripts")
        .toString();
#endif

const std::string TomoSystemSettingsLocal::g_defExternalInterpreterPath =
#ifdef _WIN32
    // assume we're using Aanconda Python and it is installed in c:/local
    // could also be c:/local/anaconda/scripts/ipython
    "c:/local/anaconda/python.exe";
#else
    // just use the system Python, assuming is in the system path
    "python";
#endif

/**
 * Default initialization which should be enough in most cases.
 */
TomoSystemSettingsLocal::TomoSystemSettingsLocal()
    : m_basePathTomoData(g_defBasePathTomoData),
      m_remoteDriveOrMountPoint(g_defRemoteDriveOrMountPoint),
      m_reconScriptsPath(g_defReconScriptsPath),
      m_externalInterpreterPath(g_defExternalInterpreterPath), m_processes(4),
      m_cores(4) {}

} // namespace CustomInterfaces
} // namespace MantidQt
