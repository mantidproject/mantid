// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "GroupHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

/**
 * Extract a vector of MatrixWorkspaces from a Workspace pointer.
 * Returns a length 1 list if the input is a MatrixWorkspace already.
 * Otherwise extracts the MatrixWorkspaces from the group.
 *
 * @param ws A MatrixWorkspace or WorkspaceGroup of MatrixWorkspaces.
 * @return a vector containing the MatrixWorkspaces present in the input.
 */
std::vector<Mantid::API::MatrixWorkspace_sptr> getMembers(Mantid::API::Workspace_sptr const &ws) {
  auto matrix_ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws);
  if (matrix_ws) {
    return {matrix_ws};
  }
  auto ws_group = std::dynamic_pointer_cast<Mantid::API::WorkspaceGroup>(ws);
  if (!ws_group) {
    return {};
  }
  std::vector<Mantid::API::MatrixWorkspace_sptr> members;
  members.reserve(ws_group->size());
  auto const &allItems = ws_group->getAllItems();
  std::transform(allItems.cbegin(), allItems.cend(), std::back_inserter(members),
                 [](auto const &member) { return std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(member); });
  return members;
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
