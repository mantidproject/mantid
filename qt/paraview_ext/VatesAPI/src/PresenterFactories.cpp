#include "MantidVatesAPI/PresenterFactories.h"

namespace Mantid {
namespace VATES {

const std::string &EmptyWorkspaceNamePolicy::getWorkspaceName(
    const Mantid::API::IMDWorkspace & /*workspace*/) {
  static std::string name{"__EmptyWorkspaceNamePolicy"};
  return name;
}

} // namespace VATES
} // namespace Mantid
