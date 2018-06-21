#include "SlicedReductionWorkspaces.h"

namespace MantidQt {
namespace CustomInterfaces {

SlicedReductionWorkspaces::SlicedReductionWorkspaces(
    std::string const &inputWorkspace,
    std::vector<ReductionWorkspaces> const &sliceWorkspaces)
    : m_inputWorkspace(std::move(inputWorkspace)),
      m_sliceWorkspaces(std::move(sliceWorkspaces)) {}

std::string const &SlicedReductionWorkspaces::inputWorkspace() const {
  return m_inputWorkspace;
}

std::vector<ReductionWorkspaces> const &
SlicedReductionWorkspaces::sliceWorkspaces() const {
  return m_sliceWorkspaces;
}

SlicedReductionWorkspaces
workspaceNamesForSliced(std::vector<std::string> const &,
                        std::pair<std::string, std::string> const &,
                        Slicing const &) {
  // TODO: Implement this correctly.
  return SlicedReductionWorkspaces("", {});
}

std::string postprocessedWorkspaceNameForSliced(
    std::vector<std::vector<std::string> const *> const &, Slicing const &) {
  // TODO: Implement this correctly.
  return "";
}

bool operator==(SlicedReductionWorkspaces const &lhs,
                SlicedReductionWorkspaces const &rhs) {
  return lhs.inputWorkspace() == rhs.inputWorkspace() &&
         lhs.sliceWorkspaces() == rhs.sliceWorkspaces();
}
bool operator!=(SlicedReductionWorkspaces const &lhs,
                SlicedReductionWorkspaces const &rhs) {
  return !(lhs == rhs);
}
}
}
