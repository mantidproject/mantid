#include "MantidMuon/ApplyMuonDetectorGroupPairing.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

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

void ApplyMuonDetectorGroupPairing::init() {
  std::string emptyString("");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspaceGroup", emptyString, Direction::InOut,
          PropertyMode::Mandatory),
      "The workspace group to which the output will be added.");

  declareProperty("PairName", emptyString,
                  "The name of the pair. Must "
                  "contain at least one alphanumeric "
                  "character.",
                  Direction::Input);

  // Select groups via workspaces

  declareProperty("SpecifyGroupsManually", false,
                  "Specify the pair of groups manually using the raw data and "
                  "various optional parameters.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace1", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace2", emptyString, Direction::Input,
          PropertyMode::Optional),
      "Input workspace containing data from grouped detectors.");

  setPropertySettings(
      "InputWorkspace1",
      make_unique<Kernel::EnabledWhenProperty>("SpecifyGroupsManually",
                                               Kernel::IS_DEFAULT, ""));
  setPropertySettings(
      "InputWorkspace2",
      make_unique<Kernel::EnabledWhenProperty>("SpecifyGroupsManually",
                                               Kernel::IS_DEFAULT, ""));

  // Specify groups manually

  declareProperty(Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace", emptyString, Direction::Input,
                      PropertyMode::Optional),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");

  declareProperty("Group1", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  declareProperty("Group2", std::to_string(1),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.",
                  Direction::Input);
  setPropertySettings("Group1",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("Group2",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty(
      "TimeMin", 0.1,
      "Start time for the data in ms. Only used with the asymmetry analysis.",
      Direction::Input);
  setPropertySettings("TimeMin",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("TimeMin",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty(
      "TimeMax", 32.0,
      "End time for the data in ms. Only used with the asymmetry analysis.",
      Direction::Input);
  setPropertySettings("TimeMax",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("TimeMax",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty("RebinArgs", emptyString,
                  "Rebin arguments. No rebinning if left empty.",
                  Direction::Input);
  setPropertySettings("RebinArgs",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("RebinArgs",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty("TimeOffset", 0.0,
                  "Shift the times of all data by a fixed amount. The value "
                  "given corresponds to the bin that will become 0.0 seconds.",
                  Direction::Input);
  setPropertySettings("TimeOffset",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("TimeOffset",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty("SummedPeriods", std::to_string(1),
                  "A list of periods to sum in multiperiod data.",
                  Direction::Input);
  setPropertySettings("SummedPeriods",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("SummedPeriods",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty("SubtractedPeriods", emptyString,
                  "A list of periods to subtract in multiperiod data.",
                  Direction::Input);
  setPropertySettings("SubtractedPeriods",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("SubtractedPeriods",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty(
      "ApplyDeadTimeCorrection", false,
      "Whether dead time correction should be applied to input workspace");
  setPropertySettings("ApplyDeadTimeCorrection",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  declareProperty(
      make_unique<WorkspaceProperty<TableWorkspace>>(
          "DeadTimeTable", "", Direction::Input, PropertyMode::Optional),
      "Table with dead time information. Must be specified if "
      "ApplyDeadTimeCorrection is set true.");
  setPropertySettings("DeadTimeTable", make_unique<Kernel::EnabledWhenProperty>(
                                           "ApplyDeadTimeCorrection",
                                           Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("DeadTimeTable",
                      make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));
  setPropertySettings("DeadTimeTable",
                      make_unique<Kernel::VisibleWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_NOT_DEFAULT, ""));

  // Perform Group Associations.

  std::string workspaceGrp("Specify Group Workspaces");
  setPropertyGroup("InputWorkspace1", workspaceGrp);
  setPropertyGroup("InputWorkspace2", workspaceGrp);

  std::string manualGroupGrp("Specify Detector ID Groups Manually");
  setPropertyGroup("InputWorkspace", manualGroupGrp);
  setPropertyGroup("Group1", manualGroupGrp);
  setPropertyGroup("Group2", manualGroupGrp);
  setPropertyGroup("TimeMin", manualGroupGrp);
  setPropertyGroup("TimeMax", manualGroupGrp);
  setPropertyGroup("RebinArgs", manualGroupGrp);
  setPropertyGroup("TimeOffset", manualGroupGrp);
  setPropertyGroup("SummedPeriods", manualGroupGrp);
  setPropertyGroup("SubtractedPeriods", manualGroupGrp);
  setPropertyGroup("ApplyDeadTimeCorrection", manualGroupGrp);
  setPropertyGroup("DeadTimeTable", manualGroupGrp);
}

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

void ApplyMuonDetectorGroupPairing::exec() {
	// if manual, set up workspaces and get their names
	std::pair<std::string, std::string> names = getGroupWorkspaceNames();

	pairWorkspaceName = getPairWorkspaceName();

	createPairWorkspace(pairWorkspaceName);


}

// Allow WorkspaceGroup property to function correctly.
bool ApplyMuonDetectorGroupPairing::checkGroups() { return false; }

} // namespace Muon
} // namespace Mantid
