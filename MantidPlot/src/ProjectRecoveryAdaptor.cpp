#include <string>

#include "qstring.h"

#include "ApplicationWindow.h"
#include "globals.h"
#include "Folder.h"
#include "ProjectRecoveryAdaptor.h"
#include "ProjectSerialiser.h"

namespace MantidQt {
namespace API {

ProjectRecoveryAdaptor::ProjectRecoveryAdaptor(ApplicationWindow *windowHandle)
    : m_windowPtr(windowHandle) {}

void ProjectRecoveryAdaptor::saveOpenWindows(std::string projectFilepath) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(m_windowPtr, isRecovery);
  projectWriter.save(QString::fromStdString(projectFilepath));
}

void ProjectRecoveryAdaptor::loadOpenWindows(std::string projectFilePath) {
  const bool isRecovery = true;
  ProjectSerialiser projectWriter(m_windowPtr, isRecovery);

  // Use this version of Mantid as the current version field - as recovery
  // across major versions is not an intended use case
  const int fileVersion = 100 * maj_version + 10 * min_version + patch_version;

  projectWriter.load(projectFilePath, fileVersion);
}

} // namespace API
} // namespace MantidQt