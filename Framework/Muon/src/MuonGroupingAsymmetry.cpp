// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonGroupingAsymmetry.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"

#include "MantidMuon/MuonAlgorithmHelper.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {

bool checkPeriodInWorkspaceGroup(const int &period,
                                 WorkspaceGroup_const_sptr &workspace) {
  return period <= workspace->getNumberOfEntries();
}

/**
 * Estimate the asymmetrey for the given workspace (TF data).
 * @param inputWS :: [input] Workspace to calculate asymmetry for
 * @param index :: [input] GroupIndex (fit only the requested spectrum): use -1
 * for "unset"
 * @returns Result of the removal
 */
MatrixWorkspace_sptr estimateAsymmetry(const Workspace_sptr &inputWS,
                                       const int index, const double startX,
                                       const double endX,
                                       const double normalizationIn) {
  IAlgorithm_sptr asym = AlgorithmManager::Instance().createUnmanaged(
      "EstimateMuonAsymmetryFromCounts");
  asym->initialize();
  asym->setChild(true);
  asym->setProperty("InputWorkspace", inputWS);
  asym->setProperty("WorkspaceName", inputWS->getName());
  if (index > -1) {
    const std::vector<int> spec(1, index);
    asym->setProperty("Spectra", spec);
  }
  asym->setProperty("OutputWorkspace", "__NotUsed__");
  asym->setProperty("StartX", startX);
  asym->setProperty("EndX", endX);
  asym->setProperty("NormalizationIn", normalizationIn);
  asym->setProperty("OutputUnNormData", false);
  asym->setProperty("OutputUnNormWorkspace", "tmp_unNorm");
  asym->execute();
  MatrixWorkspace_sptr outWS = asym->getProperty("OutputWorkspace");

  return outWS;
}

Mantid::API::MatrixWorkspace_sptr estimateMuonAsymmetry(
    WorkspaceGroup_sptr inputWS, const std::vector<int> &summedPeriods,
    const std::vector<int> &subtractedPeriods, int groupIndex,
    const double startX, const double endX, const double normalizationIn) {
  MatrixWorkspace_sptr tempWS;
  int numPeriods = inputWS->getNumberOfEntries();
  if (numPeriods > 1) {

    auto summedWS =
        Mantid::MuonAlgorithmHelper::sumPeriods(inputWS, summedPeriods);
    auto subtractedWS =
        Mantid::MuonAlgorithmHelper::sumPeriods(inputWS, subtractedPeriods);

    // Remove decay (summed periods ws)
    MatrixWorkspace_sptr asymSummedPeriods =
        estimateAsymmetry(summedWS, groupIndex, startX, endX, normalizationIn);

    if (!subtractedPeriods.empty()) {
      // Remove decay (subtracted periods ws)
      MatrixWorkspace_sptr asymSubtractedPeriods = estimateAsymmetry(
          subtractedWS, groupIndex, startX, endX, normalizationIn);

      // Now subtract
      tempWS = Mantid::MuonAlgorithmHelper::subtractWorkspaces(
          asymSummedPeriods, asymSubtractedPeriods);
    } else {
      tempWS = asymSummedPeriods;
    }
  } else {
    // Only one period was supplied
    tempWS =
        estimateAsymmetry(inputWS->getItem(0), groupIndex, startX, endX,
                          normalizationIn); // change -1 to m_groupIndex and
                                            // follow through to store as a
                                            // table for later.
  }

  MatrixWorkspace_sptr outWS =
      Mantid::MuonAlgorithmHelper::extractSpectrum(tempWS, groupIndex);
  return outWS;
}

MatrixWorkspace_sptr groupDetectors(MatrixWorkspace_sptr workspace,
                                    const std::vector<int> &detectorIDs) {

  auto outputWS = WorkspaceFactory::Instance().create(workspace, 1);

  const std::vector<size_t> wsIndices =
      workspace->getIndicesFromDetectorIDs(detectorIDs);

  if (wsIndices.size() != detectorIDs.size())
    throw std::invalid_argument("Some of the detector IDs were not found");

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

} // namespace

namespace Mantid {
namespace Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonGroupingAsymmetry)

void MuonGroupingAsymmetry::init() {
  const std::string emptyString("");
  std::vector<int> defaultGrouping = {1};
  std::vector<int> defaultPeriods = {1};

  declareProperty(
      std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspace", emptyString, Direction::Input,
          PropertyMode::Mandatory),
      "Input workspace containing data from detectors which are to "
      "be grouped.");

  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", emptyString, Direction::Output),
                  "Output workspace which will hold the results of the group "
                  "asymmetry calculation.");

  declareProperty("GroupName", emptyString,
                  "The name of the group. Must "
                  "contain at least one alphanumeric "
                  "character.",
                  Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<int>>(
                      "Grouping", std::move(defaultGrouping),
                      IValidator_sptr(new NullValidator), Direction::Input),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.");

  declareProperty("AsymmetryTimeMin", 0.0,
                  "Start time for the asymmetry estimation (in micro "
                  "seconds). Defaults to the start time of the InputWorkspace.",
                  Direction::Input);

  declareProperty("AsymmetryTimeMax", 32.0,
                  "End time for the asymmetry estimation (in micro seconds). "
                  "Defaults to the end time of the InputWorkspace.",
                  Direction::Input);

  declareProperty("NormalizationIn", 0.0,
                  "If this value is non-zero then this is used for the "
                  "normalization, instead of being estimated.",
                  Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<int>>(
                      "SummedPeriods", std::move(defaultPeriods),
                      IValidator_sptr(new NullValidator), Direction::Input),
                  "A list of periods to sum in multiperiod data.");
  declareProperty(
      std::make_unique<ArrayProperty<int>>("SubtractedPeriods", Direction::Input),
      "A list of periods to subtract in multiperiod data.");

  // Perform Group Associations.

  std::string groupingGrp("Grouping Information");
  setPropertyGroup("GroupName", groupingGrp);
  setPropertyGroup("Grouping", groupingGrp);
  setPropertyGroup("AsymmetryTimeMin", groupingGrp);
  setPropertyGroup("AsymmetryTimeMax", groupingGrp);

  std::string periodGrp("Multi-period Data");
  setPropertyGroup("SummedPeriods", periodGrp);
  setPropertyGroup("SubtractedPeriods", periodGrp);
}

std::map<std::string, std::string> MuonGroupingAsymmetry::validateInputs() {
  std::map<std::string, std::string> errors;

  const std::string groupName = this->getProperty("GroupName");
  if (groupName.empty()) {
    errors["GroupName"] = "Group name must be specified.";
  }

  if (!std::all_of(std::begin(groupName), std::end(groupName),
                   Mantid::MuonAlgorithmHelper::isAlphanumericOrUnderscore)) {
    errors["GroupName"] =
        "The group name must contain alphnumeric characters and _ only.";
  }

  WorkspaceGroup_const_sptr inputWS = getProperty("InputWorkspace");
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

  if (inputWS->getNumberOfEntries() < 1) {
    errors["InputWorkspace"] = "WorkspaceGroup contains no periods.";
  }

  const double xMin = getProperty("AsymmetryTimeMin");
  const double xMax = getProperty("AsymmetryTimeMax");
  if (xMax <= xMin) {
    errors["AsymmetryTimeMin"] = "TimeMax <= TimeMin";
  }

  return errors;
}

WorkspaceGroup_sptr
MuonGroupingAsymmetry::createGroupWorkspace(WorkspaceGroup_sptr inputWS) {
  const std::vector<int> group = this->getProperty("Grouping");
  auto groupedPeriods = boost::make_shared<WorkspaceGroup>();
  // for each period
  for (auto &&workspace : *inputWS) {
    auto groupWS = groupDetectors(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), group);
    groupedPeriods->addWorkspace(groupWS);
  }
  return groupedPeriods;
}

void MuonGroupingAsymmetry::exec() {

  WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outWS;

  const double startX = getProperty("AsymmetryTimeMin");
  const double endX = getProperty("AsymmetryTimeMax");
  const double normalizationIn = getProperty("NormalizationIn");

  const std::vector<int> summedPeriods = getProperty("SummedPeriods");
  const std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");

  WorkspaceGroup_sptr groupedWS = createGroupWorkspace(inputWS);

  outWS = estimateMuonAsymmetry(groupedWS, summedPeriods, subtractedPeriods, 0,
                                startX, endX, normalizationIn);

  addGroupingAsymmetrySampleLogs(outWS);
  setProperty("OutputWorkspace", outWS);
}

void MuonGroupingAsymmetry::addGroupingAsymmetrySampleLogs(
    MatrixWorkspace_sptr workspace) {
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_asymmetry_group_name",
                                    getPropertyValue("GroupName"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_asymmetry_group",
                                    getPropertyValue("Grouping"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_asymmetry_x_min",
                                    getPropertyValue("AsymmetryTimeMin"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_asymmetry_x_max",
                                    getPropertyValue("AsymmetryTimeMax"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_summed",
                                    getPropertyValue("SummedPeriods"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_subtracted",
                                    getPropertyValue("SubtractedPeriods"));
}

} // namespace Muon
} // namespace Mantid
