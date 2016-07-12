//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CopyDetectorMapping.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumDetectorMapping.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CopyDetectorMapping)

using namespace Kernel;
using namespace API;

void CopyDetectorMapping::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "WorkspaceToMatch", "", Direction::Input));

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "WorkspaceToRemap", "", Direction::InOut));

  declareProperty(
      make_unique<PropertyWithValue<bool>>("IndexBySpectrumNumber", false,
                                           Direction::Input),
      "Will use mapping indexed by spectrum number rather than the default of"
      "Workspace Index (recommended when both workspaces have a vertical axis "
      "in spectrum number).");
}

void CopyDetectorMapping::exec() {
  MatrixWorkspace_const_sptr wsToMatch = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr wsToRemap = getProperty("WorkspaceToRemap");
  bool indexBySpecNumber = getProperty("IndexBySpectrumNumber");

  // Copy detector mapping
  SpectrumDetectorMapping detMap(wsToMatch.get(), indexBySpecNumber);
  wsToRemap->updateSpectraUsing(detMap);

  setProperty("WorkspaceToRemap", wsToRemap);
}

std::map<std::string, std::string> CopyDetectorMapping::validateInputs() {
  std::map<std::string, std::string> issues;

  MatrixWorkspace_sptr wsToMatch = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr wsToRemap = getProperty("WorkspaceToRemap");

  // Check that the workspaces actually are MatrixWorkspaces
  bool validWorkspaces = true;

  if (wsToMatch == nullptr) {
    issues["WorkspaceToMatch"] = "Must be a MatrixWorkspace";
    validWorkspaces = false;
  }

  if (wsToRemap == nullptr) {
    issues["WorkspaceToRemap"] = "Must be a MatrixWorkspace";
    validWorkspaces = false;
  }

  // Check histohram counts match (assuming both are MatrixWorkspaces)
  if (validWorkspaces &&
      wsToMatch->getNumberHistograms() != wsToRemap->getNumberHistograms())
    issues["WorkspaceToRemap"] =
        "Number of histograms must match WorkspaceToMatch";

  return issues;
}

} // namespace Algorithms
} // namespace Mantid
