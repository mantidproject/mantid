#include "MantidMuon/MuonGroupingCounts.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/System.h"
#include "MantidMuon/MuonAlgorithmHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::HistogramData;

namespace {

MatrixWorkspace_sptr groupDetectors(MatrixWorkspace_sptr workspace,
                                    const std::vector<int> &detectorIDs) {

  auto outputWS = WorkspaceFactory::Instance().create(workspace, 1);

  std::vector<size_t> wsIndices =
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
DECLARE_ALGORITHM(MuonGroupingCounts)

void MuonGroupingCounts::init() {
  std::string emptyString("");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<WorkspaceGroup>>(
          "InputWorkspace", emptyString, Direction::Input,
          PropertyMode::Mandatory),
      "Input workspace containing data from detectors which are to "
      "be grouped.");

  declareProperty(Mantid::Kernel::make_unique<WorkspaceProperty<Workspace>>(
                      "OutputWorkspace", emptyString, Direction::Output),
                  "Output workspace which will hold the grouped data.");

  declareProperty("GroupName", emptyString,
                  "The name of the group. Must contain at least one "
                  "alphanumeric character.",
                  Direction::Input);
  declareProperty(make_unique<ArrayProperty<int>>("Grouping", Direction::Input),
                  "The grouping of detectors, comma separated list of detector "
                  "IDs or hyphenated ranges of IDs.");

  declareProperty(
      make_unique<ArrayProperty<int>>("SummedPeriods", Direction::Input),
      "A list of periods to sum in multiperiod data.");
  declareProperty(
      make_unique<ArrayProperty<int>>("SubtractedPeriods", Direction::Input),
      "A list of periods to subtract in multiperiod data.");

  // Perform Group Associations.

  std::string groupingGrp("Grouping Information");
  setPropertyGroup("GroupName", groupingGrp);
  setPropertyGroup("Grouping", groupingGrp);

  std::string periodGrp("Multi-period Data");
  setPropertyGroup("SummedPeriods", periodGrp);
  setPropertyGroup("SubtractedPeriods", periodGrp);
}

void MuonGroupingCounts::exec() {

  WorkspaceGroup_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS;

  // Group detectors in each period
  std::vector<int> group = getProperty("Grouping");
  auto groupedPeriods = boost::make_shared<WorkspaceGroup>();
  for (auto &&workspace : *inputWS) {
    groupedPeriods->addWorkspace(groupDetectors(
        boost::dynamic_pointer_cast<MatrixWorkspace>(workspace), group));
  }

  std::vector<int> summedPeriods = getProperty("SummedPeriods");
  std::vector<int> subtractedPeriods = getProperty("SubtractedPeriods");
  MatrixWorkspace_sptr addedPeriodsWS =
      Mantid::MuonAlgorithmHelper::sumPeriods(groupedPeriods, summedPeriods);
  if (!subtractedPeriods.empty()) {
    MatrixWorkspace_sptr subtractedPeriodsWS =
        Mantid::MuonAlgorithmHelper::sumPeriods(groupedPeriods,
                                                subtractedPeriods);
    outputWS = Mantid::MuonAlgorithmHelper::subtractWorkspaces(
        addedPeriodsWS, subtractedPeriodsWS);
  } else {
    outputWS = addedPeriodsWS;
  }

  setGroupingSampleLogs(outputWS);
  setProperty("OutputWorkspace", outputWS);
}

void MuonGroupingCounts::setGroupingSampleLogs(MatrixWorkspace_sptr workspace) {
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group_name",
                                    getPropertyValue("GroupName"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_group",
                                    getPropertyValue("Grouping"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_summed",
                                    getPropertyValue("SummedPeriods"));
  MuonAlgorithmHelper::addSampleLog(workspace, "analysis_periods_subtracted",
                                    getPropertyValue("SubtractedPeriods"));
}

} // namespace Muon
} // namespace Mantid
