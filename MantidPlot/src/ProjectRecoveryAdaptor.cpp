#include <string>

#include "ApplicationWindow.h"
#include "Folder.h"
#include "ProjectRecoveryAdaptor.h"

namespace MantidQt {
namespace API {

ProjectRecoveryAdaptor::ProjectRecoveryAdaptor() {}

bool ProjectRecoveryAdaptor::saveOpenWindows() { return false; }

bool ProjectRecoveryAdaptor::loadOpenWindows(std::string projectFilePath) { return false; }

} // namespace API
} // namespace MantidQt