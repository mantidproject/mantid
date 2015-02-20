//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CopyDetectorMapping.h"
#include "MantidAPI/SpectrumDetectorMapping.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(CopyDetectorMapping)

using namespace Kernel;
using namespace API;

void CopyDetectorMapping::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("WorkspaceToMatch", "", Direction::Input));

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("WorkspaceToRemap", "", Direction::InOut));

  declareProperty(
      new PropertyWithValue<bool>("IndexBySpectrumNumber", false, Direction::Input),
      "Will use mapping indexed by spectrum number rather than the default of"
      "spectrum index (recommended when both workspaces have a vertical axis in spectrum number).");
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

std::map<std::string, std::string> CopyDetectorMapping::validateInputs()
{
  std::map<std::string, std::string> issues;

  MatrixWorkspace_const_sptr wsToMatch = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr wsToRemap = getProperty("WorkspaceToRemap");

  // Check histohram counts match
  if(wsToMatch->getNumberHistograms() != wsToRemap->getNumberHistograms())
    issues["WorkspaceToRemap"] = "Number of histograms must match WorkspaceToMatch";

  return issues;
}

} // namespace Algorithms
} // namespace Mantid
