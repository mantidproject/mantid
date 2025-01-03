// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/MuonGroupingCounts.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidMuon/MuonAlgorithmHelper.h"
#include <boost/format.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

bool checkPeriodInWorkspaceGroup(const int &period, const WorkspaceGroup_sptr &workspace) {
  return period <= workspace->getNumberOfEntries();
}

MatrixWorkspace_sptr groupDetectors(const MatrixWorkspace_sptr &workspace, const std::vector<int> &detectorIDs) {

  auto outputWS = WorkspaceFactory::Instance().create(workspace, 1);

  std::vector<size_t> wsIndices = workspace->getIndicesFromDetectorIDs(detectorIDs);

  if (wsIndices.size() != detectorIDs.size()) {
    std::string errorMsg = str(boost::format("The number of detectors"
                                             " requested does not equal the number of detectors "
                                             "provided %1% != %2% ") %
                               wsIndices.size() % detectorIDs.size());
    throw std::invalid_argument(errorMsg);
  }

  outputWS->getSpectrum(0).clearDetectorIDs();
  outputWS->setSharedX(0, workspace->sharedX(wsIndices.front()));

  auto hist = outputWS->histogram(0);
  for (auto &wsIndex : wsIndices) {
    hist += workspace->histogram(wsIndex);
    outputWS->getSpectrum(0).addDetectorIDs(workspace->getSpectrum(wsIndex).getDetectorIDs());
  }
  outputWS->setHistogram(0, hist);
  outputWS->getSpectrum(0).setSpectrumNo(static_cast<int32_t>(1));
  return outputWS;
}

} // namespace

namespace Mantid::Muon {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MuonGroupingCounts)

void MuonGroupingCounts::init() {
  std::string emptyString("");
  std::vector<int> defaultGrouping = {1};

  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("InputWorkspace", emptyString, Direction::Input,
                                                                      PropertyMode::Mandatory),
                  "Input workspace containing data from detectors which are to "
                  "be grouped.");

  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", emptyString, Direction::Output),
                  "Output workspace which will hold the grouped data.");

  declareProperty("GroupName", emptyString,
                  "The name of the group. Must contain at least one "
                  "alphanumeric character.",
                  Direction::Input);
  declareProperty(std::make_unique<ArrayProperty<int>>("Grouping", std::move(defaultGrouping),
                                                       IValidator_sptr(new NullValidator), Direction::Input),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.");

  declareProperty(std::make_unique<ArrayProperty<int>>("SummedPeriods", std::vector<int>(1, 1),
                                                       IValidator_sptr(new NullValidator), Direction::Input),
                  "A list of periods to sum in multiperiod data.");
  declareProperty(std::make_unique<ArrayProperty<int>>("SubtractedPeriods", Direction::Input),
                  "A list of periods to subtract in multiperiod data.");

  // Perform Group Associations.

  std::string groupingGrp("Grouping Information");
  setPropertyGroup("GroupName", groupingGrp);
  setPropertyGroup("Grouping", groupingGrp);

  std::string periodGrp("Multi-period Data");
  setPropertyGroup("SummedPeriods", periodGrp);
  setPropertyGroup("SubtractedPeriods", periodGrp);
}

std::map<std::string, std::string> MuonGroupingCounts::validateInputs() {
  std::map<std::string, std::string> errors;

  std::string groupName = this->getProperty("GroupName");
  if (groupName.empty()) {
    errors["GroupName"] = "Group name must be specified.";
  }

  if (!std::all_of(std::begin(groupName), std::end(groupName),
                   Mantid::MuonAlgorithmHelper::isAlphanumericOrUnderscore)) {
    errors["GroupName"] = "The group name must contain alphnumeric characters and _ only.";
  }

  WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
  std::vector<int> summedPeriods = getProperty("SummedPeriods");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");

  if (summedPeriods.empty() && subtractedPeriods.empty()) {
    errors["SummedPeriods"] = "At least one period must be specified";
  }

  if (!summedPeriods.empty()) {
    const int highestSummedPeriod = *std::max_element(summedPeriods.begin(), summedPeriods.end());
    if (!checkPeriodInWorkspaceGroup(highestSummedPeriod, inputWS)) {
      errors["SummedPeriods"] =
          "Requested period (" + std::to_string(highestSummedPeriod) + ") exceeds periods in data";
    }
    if (std::any_of(summedPeriods.begin(), summedPeriods.end(), [](const int &i) { return i < 0; })) {
      errors["SummedPeriods"] = "Requested periods must be greater that 0.";
    }
  }

  if (!subtractedPeriods.empty()) {
    const int highestSubtractedPeriod = *std::max_element(subtractedPeriods.begin(), subtractedPeriods.end());
    if (!checkPeriodInWorkspaceGroup(highestSubtractedPeriod, inputWS)) {
      errors["SubtractedPeriods"] =
          "Requested period (" + std::to_string(highestSubtractedPeriod) + ") exceeds periods in data";
    }
    if (std::any_of(subtractedPeriods.begin(), subtractedPeriods.end(), [](const int &i) { return i < 0; })) {
      errors["SubtractedPeriods"] = "Requested periods must be greater that 0.";
    }
  }

  if (inputWS->getNumberOfEntries() < 1) {
    errors["InputWorkspace"] = "WorkspaceGroup contains no periods.";
  }

  return errors;
}

void MuonGroupingCounts::exec() {

  WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS;

  // Group detectors in each period
  std::vector<int> group = getProperty("Grouping");
  auto groupedPeriods = std::make_shared<WorkspaceGroup>();
  for (auto &&workspace : *inputWS) {
    groupedPeriods->addWorkspace(groupDetectors(std::dynamic_pointer_cast<MatrixWorkspace>(workspace), group));
  }

  std::vector<int> summedPeriods = getProperty("SummedPeriods");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");
  MatrixWorkspace_sptr addedPeriodsWS = Mantid::MuonAlgorithmHelper::sumPeriods(groupedPeriods, summedPeriods);
  if (!subtractedPeriods.empty()) {
    MatrixWorkspace_sptr subtractedPeriodsWS =
        Mantid::MuonAlgorithmHelper::sumPeriods(groupedPeriods, subtractedPeriods);
    outputWS = Mantid::MuonAlgorithmHelper::subtractWorkspaces(addedPeriodsWS, subtractedPeriodsWS);
  } else {
    outputWS = addedPeriodsWS;
  }

  setGroupingSampleLogs(outputWS);
  setProperty("OutputWorkspace", outputWS);
}

void MuonGroupingCounts::setGroupingSampleLogs(const MatrixWorkspace_sptr &workspace) {
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group_name", getPropertyValue("GroupName"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group", getPropertyValue("Grouping"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_summed", getPropertyValue("SummedPeriods"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_subtracted", getPropertyValue("SubtractedPeriods"));
}

} // namespace Mantid::Muon
