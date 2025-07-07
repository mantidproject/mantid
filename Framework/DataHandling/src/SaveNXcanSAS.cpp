// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SaveNXcanSAS.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/SaveNXcanSASHelper.h"

using namespace Mantid::API;

namespace Mantid::DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveNXcanSAS)

/// constructor
SaveNXcanSAS::SaveNXcanSAS() = default;

void SaveNXcanSAS::init() { initStandardProperties(); }

std::map<std::string, std::string> SaveNXcanSAS::validateInputs() { return validateStandardInputs(); }

bool SaveNXcanSAS::checkGroups() {
  const Mantid::API::Workspace_sptr &workspace = getProperty("InputWorkspace");
  if (workspace && workspace->isGroup())
    return true;
  return false;
}

bool SaveNXcanSAS::processGroups() {
  Mantid::API::Workspace_sptr &&workspace = getProperty("InputWorkspace");
  auto const &group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)->getAllItems();
  std::ranges::for_each(group.cbegin(), group.cend(), [&](auto const &wsChild) {
    m_workspaces.push_back(std::dynamic_pointer_cast<MatrixWorkspace>(wsChild));
  });

  processAllWorkspaces();
  return true;
}

void SaveNXcanSAS::processAllWorkspaces() {
  m_progress = std::make_unique<API::Progress>(this, 0.1, 1.0, 3 * m_workspaces.size());
  auto baseFilename = getPropertyValue("Filename");
  for (size_t wksIndex = 0; wksIndex < m_workspaces.size(); wksIndex++) {
    saveSingleWorkspaceFile(m_workspaces.at(wksIndex),
                            NXcanSAS::prepareFilename(baseFilename, m_workspaces.size() > 1, wksIndex));
  }
}

void SaveNXcanSAS::exec() {
  Mantid::API::Workspace_sptr &&workspace = getProperty("InputWorkspace");
  m_workspaces.push_back(std::dynamic_pointer_cast<MatrixWorkspace>(workspace));

  processAllWorkspaces();
}

} // namespace Mantid::DataHandling
