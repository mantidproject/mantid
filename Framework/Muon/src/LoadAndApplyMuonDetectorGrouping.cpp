#include "MantidMuon/LoadAndApplyMuonDetectorGrouping.h"
#include "MantidMuon/ApplyMuonDetectorGrouping.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
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

namespace {} // namespace

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::MuonAlgorithmHelper;

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadAndApplyMuonDetectorGrouping)

void LoadAndApplyMuonDetectorGrouping::init() {

  declareProperty(
      Mantid::Kernel::make_unique<FileProperty>(
          "Filename", "", API::FileProperty::Load, ".xml"),
      "The XML file containing the grouping and pairing information");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
          "InputWorkspace", "", Direction::Input, PropertyMode::Mandatory),
      "Input workspace containing data from detectors that the "
      "grouping/pairing will be applied to.");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "WorkspaceGroup", "", Direction::Input, PropertyMode::Optional),
      "The workspaces created by the algorithm will be placed inside this "
      "group. If not specified will save to \"MuonAnalysisGroup\" ");
}

/**
 * Performs validation of inputs to the algorithm.
 * -
 */
std::map<std::string, std::string>
LoadAndApplyMuonDetectorGrouping::validateInputs() {
  std::map<std::string, std::string> errors;

  return errors;
}

void LoadAndApplyMuonDetectorGrouping::exec() {

  Grouping grouping = loadGroupsAndPairs();

  Workspace_sptr inputWS = getProperty("InputWorkspace");
  WorkspaceGroup_sptr groupedWS = getProperty("WorkspaceGroup");

  addGroupingToGroupWorkspace(grouping, inputWS, groupedWS);
  addPairingToGroupWorkspace(grouping, inputWS, groupedWS);

}

API::Grouping LoadAndApplyMuonDetectorGrouping::loadGroupsAndPairs() {
  Grouping grouping;
  std::string filename = getProperty("Filename");
  GroupingLoader::loadGroupingFromXML(filename, grouping);
  return grouping;
};

void LoadAndApplyMuonDetectorGrouping::addGroupingToGroupWorkspace(
    const Mantid::API::Grouping &grouping, Mantid::API::Workspace_sptr ws,
    Mantid::API::WorkspaceGroup_sptr wsGrouped) {

  size_t numGroups = grouping.groups.size();
  for (auto i = 0; i < numGroups; i++) {
    IAlgorithm_sptr alg =
        this->createChildAlgorithm("ApplyMuonDetectorGrouping");
    alg->setProperty("InputWorkspace", ws->getName());
    alg->setProperty("InputWorkspaceGroup", wsGrouped->getName());
    alg->setProperty("GroupName", grouping.groupNames[i]);
    alg->setProperty("Grouping", grouping.groups[i]);
    alg->execute();
  }
};

void LoadAndApplyMuonDetectorGrouping::addPairingToGroupWorkspace(
    const Mantid::API::Grouping &grouping, Mantid::API::Workspace_sptr ws,
    Mantid::API::WorkspaceGroup_sptr wsGrouped){

};

// Allow WorkspaceGroup property to function correctly.
bool LoadAndApplyMuonDetectorGrouping::checkGroups() { return false; }

} // namespace Muon
} // namespace Mantid
