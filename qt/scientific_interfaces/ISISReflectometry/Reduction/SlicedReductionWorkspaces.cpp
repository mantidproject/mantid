#include "SlicedReductionWorkspaces.h"

namespace MantidQt {
namespace CustomInterfaces {

SlicedReductionWorkspaces::SlicedReductionWorkspaces(
    std::string inputWorkspace,
    std::vector<ReductionWorkspaces> sliceWorkspaces)
    : m_inputWorkspace(std::move(inputWorkspace)),
      m_sliceWorkspaces(std::move(sliceWorkspaces)) {}

std::string const &SlicedReductionWorkspaces::inputWorkspace() const {
  return m_inputWorkspace;
}

std::vector<ReductionWorkspaces> const &SlicedReductionWorkspaces::sliceWorkspaces() const {
  return m_sliceWorkspaces;
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
