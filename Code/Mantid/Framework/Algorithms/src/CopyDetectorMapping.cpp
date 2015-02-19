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
      new WorkspaceProperty<>("WorkspaceToMatch", "", Direction::Input));

  declareProperty(
      new WorkspaceProperty<>("WorkspaceToRemap", "", Direction::InOut));

  declareProperty(
      new PropertyWithValue<bool>("IndexBySpectrumNumber", false, Direction::Input),
      "Will use mapping indexed by spectrum number rather than the default of"
      "spectrum index (typically not recommended).");
}

void CopyDetectorMapping::exec() {
  MatrixWorkspace_const_sptr wsToMatch = getProperty("WorkspaceToMatch");
  MatrixWorkspace_sptr wsToRemap = getProperty("WorkspaceToRemap");
  bool indexBySpecNumber = getProperty("IndexBySpectrumNumber");

  SpectrumDetectorMapping detMap(wsToMatch.get(), indexBySpecNumber);
  wsToRemap->updateSpectraUsing(detMap);

  setProperty("WorkspaceToRemap", wsToRemap);
}

} // namespace Algorithms
} // namespace Mantid
