// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/DgsRemap.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/GroupingWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(DgsRemap)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string DgsRemap::name() const { return "DgsRemap"; }

/// Algorithm's version for identification. @see Algorithm::version
int DgsRemap::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string DgsRemap::category() const { return "Workflow\\Inelastic"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void DgsRemap::init() {
  this->declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "An input workspace to mask and group.");
  this->declareProperty(
      make_unique<WorkspaceProperty<>>("MaskWorkspace", "", Direction::Input,
                                       PropertyMode::Optional),
      "A workspace containing masking information.");
  this->declareProperty(
      make_unique<WorkspaceProperty<>>(
          "GroupingWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A workspace containing grouping information");
  this->declareProperty(make_unique<FileProperty>("OldGroupingFile", "",
                                                  FileProperty::OptionalLoad),
                        "Name of an old grouping format (not XML) file.");
  this->declareProperty("ExecuteOppositeOrder", false,
                        "Execute grouping before masking.");
  this->declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                         Direction::Output),
                        "The resulting workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void DgsRemap::exec() {
  MatrixWorkspace_sptr inputWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = this->getProperty("OutputWorkspace");

  bool runOpposite = this->getProperty("ExecuteOppositeOrder");
  if (!runOpposite) {
    this->execMasking(inputWS);
    this->execGrouping(inputWS, outputWS);
  } else {
    this->execGrouping(inputWS, outputWS);
    this->execMasking(inputWS);
  }

  this->setProperty("OutputWorkspace", outputWS);
}

void DgsRemap::execMasking(MatrixWorkspace_sptr iWS) {
  MatrixWorkspace_sptr maskWS = this->getProperty("MaskWorkspace");
  if (maskWS) {
    IAlgorithm_sptr mask = this->createChildAlgorithm("MaskDetectors");
    mask->setProperty("Workspace", iWS);
    mask->setProperty("MaskedWorkspace", maskWS);
    mask->executeAsChildAlg();
  }
}

void DgsRemap::execGrouping(MatrixWorkspace_sptr iWS,
                            MatrixWorkspace_sptr &oWS) {
  MatrixWorkspace_sptr groupWS = this->getProperty("GroupingWorkspace");
  std::string oldGroupingFile = this->getProperty("OldGroupingFile");
  if (groupWS && !oldGroupingFile.empty()) {
    throw std::runtime_error(
        "Choose either GroupingWorkspace or OldGroupingFile property!");
  }

  if (groupWS || !oldGroupingFile.empty()) {
    IAlgorithm_sptr group = this->createChildAlgorithm("GroupDetectors");
    group->setProperty("InputWorkspace", iWS);
    group->setProperty("OutputWorkspace", iWS);
    if (groupWS) {
      int64_t ngroups = 0;
      std::vector<int> groupDetIdList;
      GroupingWorkspace_sptr gWS =
          boost::dynamic_pointer_cast<GroupingWorkspace>(groupWS);
      gWS->makeDetectorIDToGroupVector(groupDetIdList, ngroups);
      group->setProperty("DetectorList", groupDetIdList);
    }
    if (!oldGroupingFile.empty()) {
      group->setProperty("MapFile", oldGroupingFile);
    }
    group->setProperty("Behaviour", "Average");
    group->executeAsChildAlg();
    oWS = group->getProperty("OutputWorkspace");
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
