// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonPairingAsymmetry.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidMuon/MuonAlgorithmHelper.h"
#include <boost/format.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {
bool checkPeriodInWorkspaceGroup(const int &period,
                                 WorkspaceGroup_const_sptr workspace) {
  return period <= workspace->getNumberOfEntries();
}

int countPeriods(Workspace_const_sptr ws) {
  if (auto tmp = boost::dynamic_pointer_cast<const WorkspaceGroup>(ws)) {
    return tmp->getNumberOfEntries();
  } else {
    return 1;
  }
}

bool checkConsistentPeriods(Workspace_const_sptr ws1,
                            Workspace_const_sptr ws2) {
  if (ws1->isGroup()) {
    if (!ws2->isGroup()) {
      return false;
    }
    if (countPeriods(ws1) != countPeriods(ws2)) {
      return false;
    }
  }
  return true;
}

MatrixWorkspace_sptr getWorkspace(WorkspaceGroup_sptr group, const int &index) {
  auto ws = group->getItem(index);
  return boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
}

MatrixWorkspace_sptr groupDetectors(MatrixWorkspace_sptr workspace,
                                    const std::vector<int> &detectorIDs) {

  auto outputWS = WorkspaceFactory::Instance().create(workspace, 1);

  std::vector<size_t> wsIndices =
      workspace->getIndicesFromDetectorIDs(detectorIDs);

  if (wsIndices.size() != detectorIDs.size())
    throw std::invalid_argument(
        str(boost::format("The number of detectors requested does not equal"
                          "the number of detectors provided %1% != %2%") %
            wsIndices.size() % detectorIDs.size()));

  outputWS->getSpectrum(0).clearDetectorIDs();
  outputWS->setSharedX(0, workspace->sharedX(wsIndices.front()));

  auto hist = outputWS->histogram(0);
  for (auto &wsIndex : wsIndices) {
    hist += workspace->histogram(wsIndex);
    outputWS->getSpectrum(0).addDetectorIDs(
        workspace->getSpectrum(wsIndex).getDetectorIDs());
  }
  outputWS->setHistogram(0, hist);
  outputWS->getSpectrum(0).setSpectrumNo(static_cast<int32_t>(1));
  return outputWS;
}

// Convert a Workspace_sptr (which may be single period, MatrixWorkspace, or
// multi period WorkspaceGroup) to a WorkspaceGroup_sptr
WorkspaceGroup_sptr workspaceToWorkspaceGroup(Workspace_sptr workspace) {

  WorkspaceGroup_sptr ws1;
  if (workspace->isGroup()) {
    ws1 = boost::dynamic_pointer_cast<WorkspaceGroup>(workspace);
  } else {
    ws1 = boost::make_shared<WorkspaceGroup>();
    ws1->addWorkspace(boost::dynamic_pointer_cast<MatrixWorkspace>(workspace));
  }
  return ws1;
}

} // namespace

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonPairingAsymmetry)

void MuonPairingAsymmetry::init() {
  std::string emptyString("");
  std::vector<int> defaultGrouping1 = {1};
  std::vector<int> defaultGrouping2 = {2};

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", emptyString, Direction::Output),
                  "The workspace which will hold the results of the asymmetry "
                  "calculation.");

  declareProperty("PairName", emptyString,
                  "The name of the pair. Must "
                  "contain at least one alphanumeric "
                  "character.",
                  Direction::Input);

  declareProperty(
      "Alpha", 1.0, boost::make_shared<MandatoryValidator<double>>(),
      "Alpha parameter used in the asymmetry calculation.", Direction::Input);

  declareProperty("SpecifyGroupsManually", false,
                  "Specify the pair of groups manually");

  // Select groups via workspaces

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace1", emptyString, Direction::Input,
                      PropertyMode::Optional),
                  "Input workspace containing data from grouped detectors.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                      "InputWorkspace2", emptyString, Direction::Input,
                      PropertyMode::Optional),
                  "Input workspace containing data from grouped detectors.");

  setPropertySettings("InputWorkspace1",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));
  setPropertySettings("InputWorkspace2",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "0"));

  // Specify groups manually

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "InputWorkspace", emptyString, Direction::Input,
                      PropertyMode::Optional),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");
  setPropertySettings("InputWorkspace",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));
  declareProperty(std::make_unique<ArrayProperty<int>>(
                      "Group1", std::move(defaultGrouping1),
                      IValidator_sptr(new NullValidator), Direction::Input),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.");
  declareProperty(std::make_unique<ArrayProperty<int>>(
                      "Group2", std::move(defaultGrouping2),
                      IValidator_sptr(new NullValidator), Direction::Input),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.");
  setPropertySettings("Group1",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));
  setPropertySettings("Group2",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(std::make_unique<ArrayProperty<int>>("SummedPeriods", "1"),
                  "A list of periods to sum in multiperiod data.");
  setPropertySettings("SummedPeriods",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  declareProperty(std::make_unique<ArrayProperty<int>>("SubtractedPeriods",
                                                       Direction::Input),
                  "A list of periods to subtract in multiperiod data.");
  setPropertySettings("SubtractedPeriods",
                      std::make_unique<Kernel::EnabledWhenProperty>(
                          "SpecifyGroupsManually", Kernel::IS_EQUAL_TO, "1"));

  // Group common entries in the interface for clarity.
  const std::string workspaceGrp("Specify Group Workspaces");
  setPropertyGroup("InputWorkspace1", workspaceGrp);
  setPropertyGroup("InputWorkspace2", workspaceGrp);

  const std::string manualGroupGrp("Specify Detector ID Groups Manually");
  setPropertyGroup("InputWorkspace", manualGroupGrp);
  setPropertyGroup("Group1", manualGroupGrp);
  setPropertyGroup("Group2", manualGroupGrp);

  const std::string periodGrp("Multi-period Data");
  setPropertyGroup("SummedPeriods", periodGrp);
  setPropertyGroup("SubtractedPeriods", periodGrp);
}

std::map<std::string, std::string> MuonPairingAsymmetry::validateInputs() {
  std::map<std::string, std::string> errors;

  // Pair name must be given, and must only contain characters, digits and "_"
  const std::string pairName = this->getProperty("PairName");
  if (pairName.empty()) {
    errors["PairName"] = "Pair name must be specified.";
  }
  if (!std::all_of(std::begin(pairName), std::end(pairName),
                   MuonAlgorithmHelper::isAlphanumericOrUnderscore)) {
    errors["PairName"] =
        "The pair name must contain alphnumeric characters and _ only.";
  }

  double alpha = this->getProperty("Alpha");
  if (alpha < 0.0) {
    errors["Alpha"] = "Alpha must be non-negative.";
  }

  if (this->getProperty("SpecifyGroupsManually")) {
    validateManualGroups(errors);
  } else {
    validateGroupsWorkspaces(errors);
  }

  return errors;
}

// Validation on the parameters given if
// "SpecifyGroupsManually" is true.
void MuonPairingAsymmetry::validateManualGroups(
    std::map<std::string, std::string> &errors) {

  std::vector<int> group1 = this->getProperty("Group1");
  std::vector<int> group2 = this->getProperty("Group2");
  if (group1.empty()) {
    errors["Group1"] =
        "A valid grouping must be supplied (e.g. \"1,2,3,4,5\").";
  }
  if (group2.empty()) {
    errors["Group2"] =
        "A valid grouping must be supplied (e.g. \"1,2,3,4,5\").";
  }

  if (group1 == group2) {
    errors["Group1"] = "The two groups must be different.";
  }

  WorkspaceGroup_sptr inputWS = this->getProperty("InputWorkspace");
  validatePeriods(inputWS, errors);
}

void MuonPairingAsymmetry::validateGroupsWorkspaces(
    std::map<std::string, std::string> &errors) {
  Workspace_sptr ws1 = this->getProperty("InputWorkspace1");
  Workspace_sptr ws2 = this->getProperty("InputWorkspace2");
  if (ws1->isGroup() && !ws2->isGroup()) {
    errors["InputWorkspace1"] =
        "InputWorkspace2 should be multi period to match InputWorkspace1";
  }
  if (ws2->isGroup() && !ws1->isGroup()) {
    errors["InputWorkspace2"] =
        "InputWorkspace1 should be multi period to match InputWorkspace2";
  }
  if (!checkConsistentPeriods(ws1, ws2)) {
    errors["InputWorkspace1"] = "InputWorkspace1 and InputWorkspace2 have "
                                "inconsistent numbers of periods.";
  }
  if (ws1->isGroup() && ws2->isGroup()) {
    validatePeriods(boost::dynamic_pointer_cast<WorkspaceGroup>(ws1), errors);
    validatePeriods(boost::dynamic_pointer_cast<WorkspaceGroup>(ws2), errors);
  }
}

bool MuonPairingAsymmetry::checkGroups() { return false; }

void MuonPairingAsymmetry::exec() {

  MatrixWorkspace_sptr outWS;

  if (getProperty("SpecifyGroupsManually")) {
    outWS = execSpecifyGroupsManually();
  } else {
    outWS = execGroupWorkspaceInput();
  }

  // outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS);

  setPairAsymmetrySampleLogs(outWS);
  if (!outWS->isGroup()) {
    setProperty("OutputWorkspace", outWS);
  }
}

MatrixWorkspace_sptr MuonPairingAsymmetry::execGroupWorkspaceInput() {

  // Get the input workspace into a useful form
  Workspace_sptr tmpWS1 = getProperty("InputWorkspace1");
  Workspace_sptr tmpWS2 = getProperty("InputWorkspace2");
  WorkspaceGroup_sptr ws1 = workspaceToWorkspaceGroup(tmpWS1);
  WorkspaceGroup_sptr ws2 = workspaceToWorkspaceGroup(tmpWS2);
  WorkspaceGroup_sptr groupedPeriods = boost::make_shared<WorkspaceGroup>();
  for (int i = 0; i < countPeriods(ws1); i++) {
    groupedPeriods->addWorkspace(
        appendSpectra(getWorkspace(ws1, i), getWorkspace(ws2, i)));
  }

  // Do the asymmetry calculation
  const double alpha = static_cast<double>(getProperty("Alpha"));
  std::vector<int> summedPeriods = getProperty("SummedPeriods");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");
  return calcPairAsymmetryWithSummedAndSubtractedPeriods(
      summedPeriods, subtractedPeriods, groupedPeriods, alpha);
}

MatrixWorkspace_sptr MuonPairingAsymmetry::execSpecifyGroupsManually() {

  WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
  auto groupedPeriods = createGroupWorkspace(inputWS);

  // Do the asymmetry calculation
  const std::vector<int> summedPeriods = getProperty("SummedPeriods");
  const std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");
  const double alpha = static_cast<double>(getProperty("Alpha"));

  return calcPairAsymmetryWithSummedAndSubtractedPeriods(
      summedPeriods, subtractedPeriods, groupedPeriods, alpha);
}

MatrixWorkspace_sptr
MuonPairingAsymmetry::calcPairAsymmetryWithSummedAndSubtractedPeriods(
    const std::vector<int> &summedPeriods,
    const std::vector<int> &subtractedPeriods,
    WorkspaceGroup_sptr groupedPeriods, const double &alpha) {
  auto summedWS =
      MuonAlgorithmHelper::sumPeriods(groupedPeriods, summedPeriods);
  auto subtractedWS =
      MuonAlgorithmHelper::sumPeriods(groupedPeriods, subtractedPeriods);

  MatrixWorkspace_sptr asymSummedPeriods = pairAsymmetryCalc(summedWS, alpha);

  if (subtractedPeriods.empty()) {
    return asymSummedPeriods;
  }

  MatrixWorkspace_sptr asymSubtractedPeriods =
      pairAsymmetryCalc(subtractedWS, alpha);

  return MuonAlgorithmHelper::subtractWorkspaces(asymSummedPeriods,
                                                 asymSubtractedPeriods);
}

/*
Create a WorkspaceGroup containing one or more periods; for each period the
workspace has two spectra corresponding to the two groupings specified in the
inputs.
*/
WorkspaceGroup_sptr
MuonPairingAsymmetry::createGroupWorkspace(WorkspaceGroup_sptr inputWS) {

  std::vector<int> group1 = this->getProperty("Group1");
  std::vector<int> group2 = this->getProperty("Group2");
  auto groupedPeriods = boost::make_shared<WorkspaceGroup>();
  // for each period
  for (auto &&workspace : *inputWS) {
    auto groupWS1 = groupDetectors(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), group1);
    auto groupWS2 = groupDetectors(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), group2);
    groupedPeriods->addWorkspace(appendSpectra(groupWS1, groupWS2));
  }
  return groupedPeriods;
}

/**
 * Performs asymmetry calculation on the given workspace using indices 0,1.
 * @param inputWS :: [input] Workspace to calculate asymmetry from (will use
 * workspace indices 0,1).
 * @returns MatrixWorkspace containing result of the calculation.
 */
MatrixWorkspace_sptr
MuonPairingAsymmetry::pairAsymmetryCalc(MatrixWorkspace_sptr inputWS,
                                        const double &alpha) {
  MatrixWorkspace_sptr outWS;

  // Ensure our specified spectra definitely point to the data
  inputWS->getSpectrum(0).setSpectrumNo(0);
  inputWS->getSpectrum(1).setSpectrumNo(1);
  const std::vector<int> fwdSpectra = {0};
  const std::vector<int> bwdSpectra = {1};

  IAlgorithm_sptr alg = this->createChildAlgorithm("AsymmetryCalc");
  alg->setProperty("InputWorkspace", inputWS);
  alg->setProperty("ForwardSpectra", fwdSpectra);
  alg->setProperty("BackwardSpectra", bwdSpectra);
  alg->setProperty("Alpha", alpha);
  alg->setProperty("OutputWorkspace", "__NotUsed__");
  alg->execute();
  outWS = alg->getProperty("OutputWorkspace");

  return outWS;
}

void MuonPairingAsymmetry::setPairAsymmetrySampleLogs(
    MatrixWorkspace_sptr workspace) {
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_pairName",
                                    getProperty("PairName"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_alpha",
                                    getProperty("Alpha"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group1",
                                    getProperty("Group1"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group2",
                                    getProperty("Group2"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_summed",
                                    getPropertyValue("SummedPeriods"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_subtracted",
                                    getPropertyValue("SubtractedPeriods"));
}

MatrixWorkspace_sptr
MuonPairingAsymmetry::appendSpectra(MatrixWorkspace_sptr inputWS1,
                                    MatrixWorkspace_sptr inputWS2) {

  IAlgorithm_sptr alg = this->createChildAlgorithm("AppendSpectra");
  alg->setProperty("InputWorkspace1", inputWS1);
  alg->setProperty("InputWorkspace2", inputWS2);
  alg->setProperty("ValidateInputs", true);
  alg->execute();
  MatrixWorkspace_sptr ws = alg->getProperty("OutputWorkspace");
  return ws;
}

void MuonPairingAsymmetry::validatePeriods(
    WorkspaceGroup_sptr inputWS, std::map<std::string, std::string> &errors) {
  const std::vector<int> summedPeriods = getProperty("SummedPeriods");
  const std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");
  if (summedPeriods.empty() && subtractedPeriods.empty()) {
    errors["SummedPeriods"] = "At least one period must be specified";
  }

  if (!summedPeriods.empty()) {
    const int highestSummedPeriod =
        *std::max_element(summedPeriods.begin(), summedPeriods.end());
    if (!checkPeriodInWorkspaceGroup(highestSummedPeriod, inputWS)) {
      errors["SummedPeriods"] = "Requested period (" +
                                std::to_string(highestSummedPeriod) +
                                ") exceeds periods in data";
    }
    if (std::any_of(summedPeriods.begin(), summedPeriods.end(),
                    [](const int &i) { return i < 0; })) {
      errors["SummedPeriods"] = "Requested periods must be greater that 0.";
    }
  }

  if (!subtractedPeriods.empty()) {
    const int highestSubtractedPeriod =
        *std::max_element(subtractedPeriods.begin(), subtractedPeriods.end());
    if (!checkPeriodInWorkspaceGroup(highestSubtractedPeriod, inputWS)) {
      errors["SubtractedPeriods"] = "Requested period (" +
                                    std::to_string(highestSubtractedPeriod) +
                                    ") exceeds periods in data";
    }
    if (std::any_of(subtractedPeriods.begin(), subtractedPeriods.end(),
                    [](const int &i) { return i < 0; })) {
      errors["SubtractedPeriods"] = "Requested periods must be greater that 0.";
    }
  }
}

} // namespace Muon
} // namespace Mantid
