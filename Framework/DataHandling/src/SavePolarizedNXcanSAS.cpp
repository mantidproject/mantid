// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/SavePolarizedNXcanSAS.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/NXcanSASHelper.h"

using namespace Mantid::API;

namespace Mantid::DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SavePolarizedNXcanSAS)

/// constructor
SavePolarizedNXcanSAS::SavePolarizedNXcanSAS() = default;

void SavePolarizedNXcanSAS::init() { initStandardProperties(); }

std::map<std::string, std::string> SavePolarizedNXcanSAS::validateInputs() { return validateStandardInputs(); }

bool SavePolarizedNXcanSAS::processGroups() {
  Mantid::API::Workspace_sptr &&workspace = getProperty("InputWorkspace");
  auto const &group = std::dynamic_pointer_cast<WorkspaceGroup>(workspace)->getAllItems();
  std::ranges::for_each(group.cbegin(), group.cend(), [&](auto const &wsChild) {
    m_workspaces.push_back(std::dynamic_pointer_cast<MatrixWorkspace>(wsChild));
  });

  processAllWorkspaces();
  return true;
}

void SavePolarizedNXcanSAS::processAllWorkspaces() {
  m_progress = std::make_unique<API::Progress>(this, 0.1, 1.0, 3 * m_workspaces.size());
  auto baseFilename = getPropertyValue("Filename");
  for (auto wksIndex = 0; wksIndex < static_cast<int>(m_workspaces.size()); wksIndex++) {
    saveSingleWorkspaceFile(m_workspaces.at(wksIndex),
                            NXcanSAS::prepareFilename(baseFilename, wksIndex, m_workspaces.size() > 1));
  }
}

void SavePolarizedNXcanSAS::exec() {
  Mantid::API::Workspace_sptr &&workspace = getProperty("InputWorkspace");
  m_workspaces.push_back(std::dynamic_pointer_cast<MatrixWorkspace>(workspace));

  processAllWorkspaces();
}

} // namespace Mantid::DataHandling
