// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/GenerateGoniometerIndependentBackground.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

namespace Mantid {
namespace Algorithms {
using namespace API;
using namespace Kernel;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GenerateGoniometerIndependentBackground)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string GenerateGoniometerIndependentBackground::name() const {
  return "GenerateGoniometerIndependentBackground";
}

/// Algorithm's version for identification. @see Algorithm::version
int GenerateGoniometerIndependentBackground::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string GenerateGoniometerIndependentBackground::category() const {
  return "CorrectionFunctions\\BackgroundCorrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string GenerateGoniometerIndependentBackground::summary() const {
  return "Extract the background from a dataset where sample is rotated through multiple positions";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void GenerateGoniometerIndependentBackground::init() {
  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>()),
                  "Input workspaces. Must be :ref:`EventWorkspace` and have at least 2 input workspaces.");

  const std::vector<std::string> exts{".map", ".xml"};
  declareProperty(
      std::make_unique<FileProperty>("GroupingFile", "", FileProperty::Load, exts),
      "A file that consists of lists of spectra numbers to group. To be read by :ref:`algm-LoadDetectorsGroupingFile`");

  declareProperty("PercentMin", 0.0, std::make_shared<BoundedValidator<double>>(0, 99.9),
                  "Starting percentage range of input files that will be combined to create the background");

  declareProperty("PercentMax", 20.0, std::make_shared<BoundedValidator<double>>(0.1, 100),
                  "Ending percentage range of input files that will be combined to create the background");

  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Extracted background workspace.");
}

std::map<std::string, std::string> GenerateGoniometerIndependentBackground::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::vector<std::string> inputs = RunCombinationHelper::unWrapGroups(getProperty("InputWorkspaces"));

  const double percentMin = getProperty("PercentMin");
  const double percentMax = getProperty("PercentMax");

  if (percentMin >= percentMax) {
    issues["PercentMin"] = "PercentMin must be less than PercentMax";
    issues["PercentMax"] = "PercentMin must be less than PercentMax";
  }

  if (inputs.size() < 2) {
    issues["InputWorkspaces"] = "Requires at least 2 input workspaces";
    return issues;
  }

  EventWorkspace_const_sptr firstWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputs.front());

  if (!firstWS) {
    issues["InputWorkspaces"] = "Workspace \"" + inputs.front() + "\" is not an EventWorkspace";
    return issues;
  }

  if (!firstWS->isCommonBins()) {
    issues["InputWorkspaces"] = "Workspaces require common bins.";
    return issues;
  }

  const auto numHist = firstWS->getNumberHistograms();
  const auto blocksize = firstWS->blocksize();
  const auto eventType = firstWS->getEventType();
  const auto instrumentName = firstWS->getInstrument()->getName();
  double protonChargeMin = firstWS->run().getProtonCharge();
  double protonChargeMax = firstWS->run().getProtonCharge();

  for (const auto &input : inputs) {
    EventWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(input);
    if (!ws) {
      issues["InputWorkspaces"] = "Workspace \"" + input + "\" is not an EventWorkspace";
      break;
    }

    if (ws->getNumberHistograms() != numHist) {
      issues["InputWorkspaces"] = "Number of spectra mismatch.";
      break;
    }

    if (!ws->isCommonBins()) {
      issues["InputWorkspaces"] = "Workspaces require common bins.";
      return issues;
    }

    if (ws->blocksize() != blocksize) {
      issues["InputWorkspaces"] = "Size mismatch.";
      break;
    }

    if (ws->getEventType() != eventType) {
      issues["InputWorkspaces"] = "Mismatched type of events in the EventWorkspaces.";
      break;
    }

    if (ws->getInstrument()->getName() != instrumentName) {
      issues["InputWorkspaces"] = "Instrument name mismatch.";
      break;
    }

    protonChargeMin = std::min(ws->run().getProtonCharge(), protonChargeMin);
    protonChargeMax = std::max(ws->run().getProtonCharge(), protonChargeMax);
  }

  if (issues.count("InputWorkspaces") == 0 && protonChargeMin > 0 &&
      (protonChargeMax - protonChargeMin) / protonChargeMin > 0.01)
    issues["InputWorkspaces"] = "Proton charge must not vary more than 1%";

  return issues;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void GenerateGoniometerIndependentBackground::exec() {

  const std::vector<std::string> inputs = RunCombinationHelper::unWrapGroups(getProperty("InputWorkspaces"));
  std::vector<EventWorkspace_const_sptr> inputWS;

  for (const auto &input : inputs) {
    EventWorkspace_const_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(input);
    ws->sortAll(TOF_SORT, nullptr);
    inputWS.push_back(ws);
  }

  // create output workspace
  EventWorkspace_sptr outputWS = create<EventWorkspace>(*inputWS.front());

  // Calculate number of workspaces to use for background
  const auto numInputs = inputWS.size();

  const double percentMin = getProperty("PercentMin");
  const auto minN = (size_t)(percentMin / 100 * (double)numInputs);

  const double percentMax = getProperty("PercentMax");
  const auto maxN = std::max(minN + 1, (size_t)(percentMax / 100 * (double)numInputs));

  g_log.information() << "background will use " << maxN - minN << " out of a total of " << inputWS.size()
                      << " workspaces starting from " << minN << "\n";

  auto alg = createChildAlgorithm("LoadDetectorsGroupingFile");
  alg->setProperty("InputFile", getPropertyValue("GroupingFile"));
  alg->setProperty("InputWorkspace", inputs.front());
  alg->executeAsChildAlg();
  GroupingWorkspace_sptr groupWS = alg->getProperty("OutputWorkspace");

  Progress progress(this, 0.0, 1.0, numInputs + groupWS->getTotalGroups());

  std::vector<MatrixWorkspace_sptr> grouped_inputs;
  // Run GroupDetectors on all the input workspaces
  for (auto const &input : inputs) {
    const auto msg = "Running GroupDetectors on " + input;
    progress.report(msg);
    g_log.debug(msg);

    auto group = createChildAlgorithm("GroupDetectors");
    group->setPropertyValue("InputWorkspace", input);
    group->setProperty("CopyGroupingFromWorkspace", groupWS);
    group->executeAsChildAlg();

    MatrixWorkspace_sptr output = group->getProperty("OutputWorkspace");

    grouped_inputs.push_back(std::move(output));
  }

  // all spectra for all input workspaces have same binning
  const size_t numGroups = grouped_inputs.front()->getNumberHistograms();
  const auto blocksize = grouped_inputs.front()->blocksize();
  const auto Xvalues = grouped_inputs.front()->readX(0);

  for (size_t group = 0; group < numGroups; group++) {
    const auto msg = "Processing group " + std::to_string(group) + " out of " + std::to_string(numGroups);
    progress.report(msg);
    g_log.debug(msg);

    // detectors IDs for this group
    const auto detIDlist = grouped_inputs.front()->getSpectrum(group).getDetectorIDs();
    const std::vector<detid_t> detIDlistV(detIDlist.begin(), detIDlist.end());
    // spectrum from the detector IDs
    const auto indexList = inputWS.at(0)->getIndicesFromDetectorIDs(detIDlistV);

    for (size_t x = 0; x < blocksize; x++) {
      // create pair of intensity and input index to sort for every bin in this group
      std::vector<std::pair<double, size_t>> intensity_input_map;
      for (size_t f = 0; f < numInputs; f++) {
        intensity_input_map.push_back(std::make_pair(grouped_inputs.at(f)->readY(group).at(x), f));
      }

      std::sort(intensity_input_map.begin(), intensity_input_map.end());

      const auto start = Xvalues.at(x);
      const auto end = Xvalues.at(x + 1);

      for (size_t n = minN; n < maxN; n++) {
        const auto inWS = inputWS.at(intensity_input_map.at(n).second);

        for (auto &idx : indexList) {
          const auto el = inWS->getSpectrum(idx);
          auto &outSpec = outputWS->getSpectrum(idx);

          switch (el.getEventType()) {
          case TOF:
            filterAndAddEvents(el.getEvents(), outSpec, TOF, start, end);
            break;
          case WEIGHTED:
            filterAndAddEvents(el.getWeightedEvents(), outSpec, WEIGHTED, start, end);
            break;
          case WEIGHTED_NOTIME:
            filterAndAddEvents(el.getWeightedEventsNoTime(), outSpec, WEIGHTED_NOTIME, start, end);
            break;
          }
        }
      }
    }
  }

  // scale output by number of workspaces used for the background
  outputWS /= (double)(maxN - minN);

  g_log.debug() << "Output workspace has " << outputWS->getNumberEvents() << " events\n";

  setProperty("OutputWorkspace", std::move(outputWS));
}

template <class T>
void GenerateGoniometerIndependentBackground::filterAndAddEvents(const std::vector<T> &events, EventList &outSpec,
                                                                 const EventType eventType, const double start,
                                                                 const double end) {
  outSpec.switchTo(eventType);
  for (auto &event : events) {
    if (event.tof() >= end)
      break;

    if (event.tof() >= start)
      outSpec.addEventQuickly(event);
  }
}

} // namespace Algorithms
} // namespace Mantid
