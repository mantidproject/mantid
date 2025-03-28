// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
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

void SavePolarizedNXcanSAS::init() {
  initStandardProperties();
  initPolarizedProperties();
}

std::map<std::string, std::string> SavePolarizedNXcanSAS::validateInputs() {
  std::map<std::string, std::string> results;

  auto const standardResults = validateStandardInputs();
  results.insert(standardResults.begin(), standardResults.end());

  auto const polarizedResults = validatePolarizedInputs();
  results.insert(polarizedResults.begin(), polarizedResults.end());

  return results;
}

void SavePolarizedNXcanSAS::exec() {
  m_progress = std::make_unique<API::Progress>(this, 0.1, 1.0, 4);

  auto const baseFilename = getPropertyValue("Filename");
  Workspace_sptr const workspace = getProperty("InputWorkspace");
  auto const wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  savePolarizedGroup(wsGroup, NXcanSAS::prepareFilename(baseFilename, 0, false));
}
} // namespace Mantid::DataHandling
