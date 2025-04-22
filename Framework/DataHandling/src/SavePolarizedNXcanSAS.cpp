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
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidDataHandling/NXcanSASHelper.h"
#include "MantidKernel/VectorHelper.h"

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
  const std::string spins = getProperty(NXcanSAS::PolProperties::INPUT_SPIN_STATES);
  const auto spinVec = Kernel::VectorHelper::splitStringIntoVector<std::string>(spins);

  auto const standardResults = validateStandardInputs();
  results.insert(standardResults.cbegin(), standardResults.cend());

  auto const polarizedWSResults = validatePolarizedInputWorkspace(spinVec);
  results.insert(polarizedWSResults.cbegin(), polarizedWSResults.cend());

  auto const polarizedSpinStatesResults = validateSpinStateStrings(spinVec);
  results.insert(polarizedSpinStatesResults.cbegin(), polarizedSpinStatesResults.cend());

  auto const polarizedMetadata = validatePolarizedMetadata();
  results.insert(polarizedMetadata.cbegin(), polarizedMetadata.cend());

  return results;
}

void SavePolarizedNXcanSAS::exec() {
  m_progress = std::make_unique<API::Progress>(this, 0.1, 1.0, 4);

  auto const baseFilename = getPropertyValue("Filename");
  Workspace_sptr const workspace = getProperty("InputWorkspace");
  auto const wsGroup = std::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  savePolarizedGroup(wsGroup, NXcanSAS::prepareFilename(baseFilename));
}
} // namespace Mantid::DataHandling
