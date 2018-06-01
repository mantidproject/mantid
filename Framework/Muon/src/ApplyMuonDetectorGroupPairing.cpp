#include "MantidMuon/ApplyMuonDetectorGroupPairing.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

const std::vector<std::string> g_analysisTypes = {"Counts", "Asymmetry"};

namespace {} // namespace

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MuonAlgorithmHelper;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyMuonDetectorGroupPairing)

void ApplyMuonDetectorGroupPairing::init() {}

/**
 * Performs validation of inputs to the algorithm.
 * -
 * @returns Map of parameter names to errors
 */
std::map<std::string, std::string>
ApplyMuonDetectorGroupPairing::validateInputs() {
  std::map<std::string, std::string> errors;

  return errors;
}

void ApplyMuonDetectorGroupPairing::exec() {}

} // namespace Muon
} // namespace Mantid
