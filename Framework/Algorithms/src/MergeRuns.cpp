// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MergeRuns.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"

#include "MantidTypes/SpectrumDefinition.h"

using Mantid::HistogramData::HistogramX;

namespace Mantid::Algorithms {

// Register with the algorithm factory
DECLARE_ALGORITHM(MergeRuns)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace RunCombinationOptions;

/// Initialisation method
void MergeRuns::init() {
  // declare arbitrary number of input workspaces as a list of strings at the
  // moment
  declareProperty(std::make_unique<ArrayProperty<std::string>>("InputWorkspaces", std::make_shared<ADSValidator>()),
                  "The names of the input workspaces as a list. You may "
                  "also group workspaces using the GUI or [[GroupWorkspaces]], and specify "
                  "the name of the group instead.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");
  declareProperty(SampleLogsBehaviour::TIME_SERIES_PROP, "", SampleLogsBehaviour::TIME_SERIES_DOC);
  declareProperty(SampleLogsBehaviour::LIST_PROP, "", SampleLogsBehaviour::LIST_DOC);
  declareProperty(SampleLogsBehaviour::WARN_PROP, "", SampleLogsBehaviour::WARN_DOC);
  declareProperty(SampleLogsBehaviour::WARN_TOL_PROP, "", SampleLogsBehaviour::WARN_TOL_DOC);
  declareProperty(SampleLogsBehaviour::FAIL_PROP, "", SampleLogsBehaviour::FAIL_DOC);
  declareProperty(SampleLogsBehaviour::FAIL_TOL_PROP, "", SampleLogsBehaviour::FAIL_TOL_DOC);
  declareProperty(SampleLogsBehaviour::SUM_PROP, "", SampleLogsBehaviour::SUM_DOC);
  const std::vector<std::string> rebinOptions = {REBIN_BEHAVIOUR, FAIL_BEHAVIOUR};
  declareProperty("RebinBehaviour", REBIN_BEHAVIOUR, std::make_shared<StringListValidator>(rebinOptions),
                  "Choose whether to rebin when bins are different, or fail "
                  "(fail behaviour defined in FailBehaviour option).");
  const std::vector<std::string> failBehaviourOptions = {SKIP_BEHAVIOUR, STOP_BEHAVIOUR};
  declareProperty("FailBehaviour", SKIP_BEHAVIOUR, std::make_shared<StringListValidator>(failBehaviourOptions),
                  "Choose whether to skip the file and continue, or stop and "
                  "throw and error, when encountering a failure.");
}

// @return the name of the property used to supply in input workspace(s).
std::string MergeRuns::fetchInputPropertyName() const { return "InputWorkspaces"; }

// @returns true since "InputWorkspaces" is a non-workspace array property taken
// to be the input.
bool MergeRuns::useCustomInputPropertyName() const { return true; }

/** Executes the algorithm
 *  @throw Exception::NotFoundError If an input workspace doesn't exist
 *  @throw std::invalid_argument If the input workspaces are not compatible
 */
void MergeRuns::exec() {
  // Check that all input workspaces exist and match in certain important ways
  const std::vector<std::string> inputs_orig = getProperty("InputWorkspaces");

  // This will hold the inputs, with the groups separated off
  std::vector<std::string> inputs = RunCombinationHelper::unWrapGroups(inputs_orig);
  if (inputs.size() == 1) {
    g_log.warning("Only one input workspace specified");
  }

  // First, try as event workspaces
  if (this->validateInputsForEventWorkspaces(inputs)) {
    // Yes, they are all event workspaces! ---------------------
    this->execEvent();
  } else {
    // At least one is not event workspace ----------------
    this->execHistogram(inputs);
  }
}

MatrixWorkspace_sptr MergeRuns::buildScanningOutputWorkspace(const MatrixWorkspace_sptr &outWS,
                                                             const MatrixWorkspace_sptr &addeeWS) {
  const auto numOutputSpectra = outWS->getNumberHistograms() + addeeWS->getNumberHistograms();

  MatrixWorkspace_sptr newOutWS =
      DataObjects::create<MatrixWorkspace>(*outWS, numOutputSpectra, outWS->histogram(0).binEdges());

  newOutWS->mutableComponentInfo().merge(addeeWS->componentInfo());

  if (newOutWS->detectorInfo().size() * newOutWS->detectorInfo().scanCount() ==
      outWS->detectorInfo().size() * outWS->detectorInfo().scanCount()) {
    // In this case the detector info objects were identical. We just add the
    // workspaces as we normally would for MergeRuns.
    g_log.information() << "Workspaces had identical detector scan information and were "
                           "merged.";
    return outWS + addeeWS;
  } else if (newOutWS->detectorInfo().size() * newOutWS->detectorInfo().scanCount() != numOutputSpectra) {
    throw std::runtime_error("Unexpected DetectorInfo size. Merging workspaces "
                             "with some, but not all overlapping scan "
                             "intervals is not currently supported.");
  }

  g_log.information() << "Workspaces had different, non-overlapping scan intervals "
                         "so spectra will be appended.";

  auto outSpecDefs = *(outWS->indexInfo().spectrumDefinitions());
  const auto &addeeSpecDefs = *(addeeWS->indexInfo().spectrumDefinitions());

  const auto newAddeeSpecDefs = buildScanIntervals(addeeSpecDefs, addeeWS->detectorInfo(), newOutWS->detectorInfo());

  outSpecDefs.insert(outSpecDefs.end(), newAddeeSpecDefs.begin(), newAddeeSpecDefs.end());

  auto newIndexInfo = Indexing::IndexInfo(numOutputSpectra);
  newIndexInfo.setSpectrumDefinitions(std::move(outSpecDefs));
  newOutWS->setIndexInfo(newIndexInfo);

  for (size_t i = 0; i < outWS->getNumberHistograms(); ++i)
    newOutWS->setHistogram(i, outWS->histogram(i));

  for (size_t i = 0; i < addeeWS->getNumberHistograms(); ++i)
    newOutWS->setHistogram(i + outWS->getNumberHistograms(), addeeWS->histogram(i));

  return newOutWS;
}

/** Build up addition tables for merging eventlists together.
 * Throws an error if there is any incompatibility.
 */
void MergeRuns::buildAdditionTables() {
  if (m_inEventWS.empty())
    throw std::invalid_argument("MergeRuns: No workspaces found to merge.");

  // This'll hold the addition tables.
  m_tables.clear();

  // This is the workspace against which everything will be added
  EventWorkspace_sptr lhs = m_inEventWS[0];
  auto lhs_nhist = static_cast<int>(lhs->getNumberHistograms());

  detid2index_map lhs_det_to_wi;
  try {
    lhs_det_to_wi = lhs->getDetectorIDToWorkspaceIndexMap(true);
  } catch (std::runtime_error &) {
    // If it fails, then there are some grouped detector IDs, and the map cannot
    // exist
  }

  m_outputSize = m_inEventWS[0]->getNumberHistograms();

  for (size_t workspaceNum = 1; workspaceNum < m_inEventWS.size(); workspaceNum++) {
    // Get the workspace
    EventWorkspace_sptr ews = m_inEventWS[workspaceNum];

    // An addition table is a list of pairs:
    //  First int = workspace index in the EW being added
    //  Second int = workspace index to which it will be added in the OUTPUT EW.
    //  -1 if it should add a new entry at the end.
    AdditionTable table;

    // Loop through the input workspace indices
    std::size_t nhist = ews->getNumberHistograms();
    table.reserve(nhist);
    for (int inWI = 0; inWI < static_cast<int>(nhist); inWI++) {
      // Get the set of detectors in the output
      auto &inDets = ews->getSpectrum(inWI).getDetectorIDs();

      bool done = false;

      // First off, try to match the workspace indices. Most times, this will be
      // ok right away.
      int outWI = inWI;
      if (outWI < lhs_nhist) // don't go out of bounds
      {
        auto &outDets = lhs->getSpectrum(outWI).getDetectorIDs();

        // Checks that inDets is a subset of outDets
        if (std::includes(outDets.begin(), outDets.end(), inDets.begin(), inDets.end())) {
          // We found the workspace index right away. No need to keep looking
          table.emplace_back(inWI, outWI);
          done = true;
        }
      }

      if (!done && !lhs_det_to_wi.empty() && (inDets.size() == 1)) {
        // Didn't find it. Try to use the LHS map.

        // First, we have to get the (single) detector ID of the RHS
        auto inDets_it = inDets.begin();
        detid_t rhs_detector_ID = *inDets_it;

        // Now we use the LHS map to find it. This only works if both the lhs
        // and rhs have 1 detector per pixel
        detid2index_map::const_iterator map_it = lhs_det_to_wi.find(rhs_detector_ID);
        if (map_it != lhs_det_to_wi.cend()) {
          // This is the workspace index in the LHS that matched rhs_detector_ID
          outWI = static_cast<int>(map_it->second);
        } else {
          // Did not find it!
          outWI = -1; // Marker to mean its not in the LHS.
          ++m_outputSize;
        }
        table.emplace_back(inWI, outWI);
        done = true; // Great, we did it.
      }

      if (!done) {
        // Didn't find it? Now we need to iterate through the output workspace
        // to match the detector ID.
        // NOTE: This can be SUPER SLOW!
        for (outWI = 0; outWI < lhs_nhist; outWI++) {
          const auto &outDets2 = lhs->getSpectrum(outWI).getDetectorIDs();
          // Another subset check
          if (std::includes(outDets2.begin(), outDets2.end(), inDets.begin(), inDets.end())) {
            // This one is right. Now we can stop looking.
            table.emplace_back(inWI, outWI);
            done = true;
            continue;
          }
        }
      }

      if (!done) {
        // If we reach here, not a single match was found for this set of
        // inDets.

        // TODO: should we check that none of the output ones are subsets of
        // this one?

        // So we need to add it as a new workspace index
        table.emplace_back(inWI, -1);
        ++m_outputSize;
      }
    }

    // Add this table to the list
    m_tables.emplace_back(table);

  } // each of the workspaces being added

  if (m_tables.size() != m_inEventWS.size() - 1)
    throw std::runtime_error("MergeRuns::buildAdditionTables: Mismatch between "
                             "the number of addition tables and the number of "
                             "workspaces");
}

//------------------------------------------------------------------------------------------------
/** Executes the algorithm for EventWorkspaces
 */
void MergeRuns::execEvent() {
  g_log.information() << "Creating an output EventWorkspace\n";

  // Make the addition tables, or throw an error if there was a problem.
  this->buildAdditionTables();

  // Create a new output event workspace, by copying the first WS in the list
  EventWorkspace_sptr inputWS = m_inEventWS[0];
  auto outWS = create<EventWorkspace>(*inputWS, m_outputSize, inputWS->binEdges(0));
  const auto inputSize = inputWS->getNumberHistograms();
  for (size_t i = 0; i < inputSize; ++i)
    outWS->getSpectrum(i) = inputWS->getSpectrum(i);

  int64_t n = m_inEventWS.size() - 1;
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, n);

  // Note that we start at 1, since we already have the 0th workspace
  auto current = inputSize;
  for (size_t workspaceNum = 1; workspaceNum < m_inEventWS.size(); workspaceNum++) {
    const auto &addee = *m_inEventWS[workspaceNum];
    const auto &table = m_tables[workspaceNum - 1];

    // Add all the event lists together as the table says to do
    for (auto &WI : table) {
      int64_t inWI = WI.first;
      int64_t outWI = WI.second;
      if (outWI >= 0) {
        outWS->getSpectrum(outWI) += addee.getSpectrum(inWI);
      } else {
        outWS->getSpectrum(current) = addee.getSpectrum(inWI);
        ++current;
      }
    }

    // Now we add up the runs
    outWS->mutableRun() += addee.run();

    m_progress->report();
  }

  // Set the final workspace to the output property
  setProperty("OutputWorkspace", std::move(outWS));
}

void MergeRuns::execHistogram(const std::vector<std::string> &inputs) {
  SampleLogsBehaviour::SampleLogNames logEntries = {};
  logEntries.sampleLogsSum = getPropertyValue(SampleLogsBehaviour::SUM_PROP);
  logEntries.sampleLogsTimeSeries = getPropertyValue(SampleLogsBehaviour::TIME_SERIES_PROP);
  logEntries.sampleLogsList = getPropertyValue(SampleLogsBehaviour::LIST_PROP);
  logEntries.sampleLogsWarn = getPropertyValue(SampleLogsBehaviour::WARN_PROP);
  logEntries.sampleLogsWarnTolerances = getPropertyValue(SampleLogsBehaviour::WARN_TOL_PROP);
  logEntries.sampleLogsFail = getPropertyValue(SampleLogsBehaviour::FAIL_PROP);
  logEntries.sampleLogsFailTolerances = getPropertyValue(SampleLogsBehaviour::FAIL_TOL_PROP);

  const std::string sampleLogsFailBehaviour = getProperty("FailBehaviour");

  // This gets the list of workspaces
  RunCombinationHelper combHelper;
  m_inMatrixWS = combHelper.validateInputWorkspaces(inputs, g_log);
  const auto rebinParams = checkRebinning();

  // Take the first input workspace as the first argument to the addition
  MatrixWorkspace_sptr outWS(m_inMatrixWS.front()->clone());
  if (rebinParams) {
    outWS = this->rebinInput(outWS, *rebinParams);
  }
  SampleLogsBehaviour::ParameterName parName = {MergeRunsParameter::SUM_MERGE,
                                                MergeRunsParameter::TIME_SERIES_MERGE,
                                                MergeRunsParameter::LIST_MERGE,
                                                MergeRunsParameter::WARN_MERGE,
                                                MergeRunsParameter::WARN_MERGE_TOLERANCES,
                                                MergeRunsParameter::FAIL_MERGE,
                                                MergeRunsParameter::FAIL_MERGE_TOLERANCES};
  Algorithms::SampleLogsBehaviour sampleLogsBehaviour = SampleLogsBehaviour(outWS, g_log, logEntries, parName);

  auto isScanning = outWS->detectorInfo().isScanning();

  const size_t numberOfWSs = m_inMatrixWS.size();
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numberOfWSs - 1);
  // Note that the iterator is incremented before first pass so that 1st
  // workspace isn't added to itself
  auto it = m_inMatrixWS.begin();
  for (++it; it != m_inMatrixWS.end(); ++it) {
    MatrixWorkspace_sptr addee;
    if (rebinParams) {
      addee = this->rebinInput(*it, *rebinParams);
    } else {
      addee = *it;
    }

    // Add the current workspace to the total
    // Update the sample logs
    try {
      sampleLogsBehaviour.mergeSampleLogs(*it, outWS);
      sampleLogsBehaviour.removeSampleLogsFromWorkspace(addee);
      if (isScanning)
        outWS = buildScanningOutputWorkspace(outWS, addee);
      else
        outWS = outWS + addee;
      sampleLogsBehaviour.setUpdatedSampleLogs(outWS);
      sampleLogsBehaviour.readdSampleLogToWorkspace(addee);
    } catch (std::invalid_argument &e) {
      if (sampleLogsFailBehaviour == SKIP_BEHAVIOUR) {
        g_log.error() << "Could not merge run: " << it->get()->getName() << ". Reason: \"" << e.what()
                      << "\". MergeRuns will continue but this run will be skipped.\n";
        sampleLogsBehaviour.resetSampleLogs(outWS);
      } else {
        throw std::invalid_argument(e);
      }
    }
    m_progress->report();
  }

  // Set the final workspace to the output property
  setProperty("OutputWorkspace", outWS);
}

//------------------------------------------------------------------------------------------------
/** Validate the input event workspaces
 *
 *  @param  inputWorkspaces The names of the input workspaces
 *  @throw invalid_argument if there is an incompatibility.
 *  @return true if all workspaces are event workspaces and valid. False if any
 *are not found,
 */
bool MergeRuns::validateInputsForEventWorkspaces(const std::vector<std::string> &inputWorkspaces) {

  m_inEventWS.clear();

  // TODO: Check that name of instrument matches - think that's the best
  // possible at the moment because if instrument is created from raw file it'll
  // be a different object
  // std::string instrument;

  RunCombinationHelper combHelper;

  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    // Fetch the next input workspace as an - throw an error if it's not there
    EventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWorkspaces[i]);

    if (!ws) { // Either it is not found, or it is not an EventWorkspace
      return false;
    }
    m_inEventWS.emplace_back(ws);

    // Check a few things are the same for all input workspaces
    if (i == 0) {
      combHelper.setReferenceProperties(ws);
    } else {
      std::string compatibility = combHelper.checkCompatibility(ws);
      if (!compatibility.empty()) {
        g_log.error("Input workspaces are not compatible: " + compatibility);
        throw std::invalid_argument("Input workspaces are not compatible: " + compatibility);
      }
    }
  } // for each input WS name

  // We got here: all are event workspaces
  return true;
}

/** Checks if the workspaces need to be rebinned and if so, returns the
 *  rebinning parameters for the Rebin algorithm.
 *  @return :: An optional object containing the rebinning params or none
 *  if rebinning is not needed.
 */
std::optional<std::vector<double>> MergeRuns::checkRebinning() {
  const std::string rebinBehaviour = getProperty("RebinBehaviour");
  const std::string sampleLogsFailBehaviour = getProperty("FailBehaviour");
  // To properly cover all X spans of the input workspaces, one needs to
  // sort the workspaces in ascending X before figuring out the rebinning.
  std::vector<MatrixWorkspace_sptr> inputsSortedByX(m_inMatrixWS.cbegin(), m_inMatrixWS.cend());
  std::sort(inputsSortedByX.begin(), inputsSortedByX.end(),
            [](const MatrixWorkspace_sptr &ws1, const MatrixWorkspace_sptr &ws2) {
              return ws1->x(0).front() < ws2->x(0).front();
            });
  auto it = inputsSortedByX.cbegin();
  g_log.notice() << "Using run '" << (*it)->getName() << "' as a reference to determine possible rebinning.\n";
  std::optional<std::vector<double>> rebinParams{std::nullopt};
  std::vector<double> bins{(*it)->x(0).rawData()};
  for (++it; it != inputsSortedByX.cend(); ++it) {
    if (!WorkspaceHelpers::matchingBins(*inputsSortedByX.front(), **it, true)) {
      if (rebinBehaviour != REBIN_BEHAVIOUR) {
        if (sampleLogsFailBehaviour == SKIP_BEHAVIOUR) {
          g_log.error() << "Could not merge run: " << (*it)->getName()
                        << ". Binning is different from the reference run. "
                           "MergeRuns will continue but this run will be "
                           "skipped.\n";
          m_inMatrixWS.remove(*it);
          continue;
        } else {
          throw std::invalid_argument("Could not merge run: " + (*it)->getName() +
                                      ". Binning is different from the reference run.");
        }
      }
      rebinParams = this->calculateRebinParams(bins, (*it)->x(0).rawData());
      VectorHelper::createAxisFromRebinParams(*rebinParams, bins);
    }
  }
  return rebinParams;
}

//------------------------------------------------------------------------------------------------
/** Calculates the parameters to hand to the Rebin algorithm. Specifies the new
 *  binning, bin-by-bin, to cover the full range covered by the two old
 *  binnings. In regions of overlap, the wider bins are taken.
 *  @param bins1 ::    The first bin edges
 *  @param bins2 ::    The second bin edges
 *  @return :: The rebinning parameters
 */
std::vector<double> MergeRuns::calculateRebinParams(const std::vector<double> &bins1,
                                                    const std::vector<double> &bins2) {
  std::vector<double> newParams;
  // Try to reserve memory for the worst-case scenario: two non-overlapping
  // ranges.
  newParams.reserve(1 + 2 * (bins1.size() - 1) + 2 + 2 * (bins2.size() - 1));
  // Sort by X axis which starts smaller
  bool const oldIsFirst = bins1.front() < bins2.front();
  auto const &smallerX = oldIsFirst ? bins1 : bins2;
  auto const &greaterX = oldIsFirst ? bins2 : bins1;
  double const end1 = smallerX.back();
  double const start2 = greaterX.front();
  double const end2 = greaterX.back();

  if (end1 <= start2) {
    // First case is if there's no overlap between the workspaces
    noOverlapParams(smallerX, greaterX, newParams);
  } else {
    // Add the bins up to the start of the overlap
    newParams.emplace_back(smallerX.front());
    size_t i;
    for (i = 1; smallerX[i] <= start2; ++i) {
      newParams.emplace_back(smallerX[i] - smallerX[i - 1]);
      newParams.emplace_back(smallerX[i]);
    }
    // If the range of one of the workspaces is completely within that
    // of the other, call the 'inclusion' routine.
    // Otherwise call the standard 'intersection' one.
    if (end1 < end2) {
      intersectionParams(smallerX, i, greaterX, newParams);
    } else {
      inclusionParams(smallerX, i, greaterX, newParams);
    }
  }
  return newParams;
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin paramters in the case where the two input workspaces do
 * not overlap at all.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::noOverlapParams(const HistogramX &X1, const HistogramX &X2, std::vector<double> &params) {
  // Add all the bins from the first workspace
  for (size_t i = 1; i < X1.size(); ++i) {
    params.emplace_back(X1[i - 1]);
    params.emplace_back(X1[i] - X1[i - 1]);
  }
  // Put a single bin in the 'gap' (but check first the 'gap' isn't zero)
  if (X1.back() < X2.front()) {
    params.emplace_back(X1.back());
    params.emplace_back(X2.front() - X1.back());
  }
  // Now add all the bins from the second workspace
  for (size_t j = 1; j < X2.size(); ++j) {
    params.emplace_back(X2[j - 1]);
    params.emplace_back(X2[j] - X2[j - 1]);
  }
  params.emplace_back(X2.back());
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin parameters in the case where the bins of the two
 * workspaces intersect.
 *  'Intersect' is used in the sense of two intersecting sets.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param i ::      Indicates the index in X1 immediately before the overlap
 * region starts
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::intersectionParams(const HistogramX &X1, size_t &i, const HistogramX &X2, std::vector<double> &params) {
  // First calculate the number of bins in each workspace that are in the
  // overlap region
  auto const overlapbins1 = X1.size() - i;
  auto const iterX2 = std::lower_bound(X2.cbegin(), X2.cend(), X1.back());
  if (iterX2 == X2.end()) {
    throw std::runtime_error("MergerRuns::intersectionParams: no intersection "
                             "between the histograms.");
  }
  auto const overlapbins2 = std::distance(X2.cbegin(), iterX2);
  // We want to use whichever one has the larger bins (on average)
  if (overlapbins1 < static_cast<size_t>(overlapbins2)) {
    // In this case we want the rest of the bins from the first workspace.....
    for (; i < X1.size(); ++i) {
      params.emplace_back(X1[i] - X1[i - 1]);
      params.emplace_back(X1[i]);
    }
    // Now remove the last bin & boundary
    params.pop_back();
    params.pop_back();
    // ....and then the non-overlap ones from the second workspace
    for (size_t j = overlapbins2; j < X2.size(); ++j) {
      params.emplace_back(X2[j] - params.back());
      params.emplace_back(X2[j]);
    }
  } else {
    // In this case we just have to add all the bins from the second workspace
    for (size_t j = 1; j < X2.size(); ++j) {
      params.emplace_back(X2[j] - params.back());
      params.emplace_back(X2[j]);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Calculates the rebin parameters in the case where the range of the second
 * workspace is
 *  entirely within that of the first workspace.
 *  'Inclusion' is used in the sense of a set being included in anothre.
 *  @param X1 ::     The bin boundaries from the first workspace
 *  @param i ::      Indicates the index in X1 immediately before the overlap
 * region starts
 *  @param X2 ::     The bin boundaries from the second workspace
 *  @param params :: A reference to the vector of rebinning parameters
 */
void MergeRuns::inclusionParams(const HistogramX &X1, size_t &i, const HistogramX &X2, std::vector<double> &params) {
  // First calculate the number of bins in each workspace that are in the
  // overlap region
  const auto iterX1 = std::lower_bound(X1.cbegin() + i, X1.cend(), X2.back());
  if (iterX1 == X1.cend()) {
    throw std::runtime_error("MergeRuns::inclusionParams: no overlap between the histograms");
  }
  auto const overlapbins1 = std::distance(X1.cbegin(), iterX1) - i;
  auto const overlapbins2 = X2.size() - 1;

  // In the overlap region, we want to use whichever one has the larger bins (on
  // average)
  if (overlapbins1 + 1 <= overlapbins2) {
    // In the case where the first workspace has larger bins it's easy
    // - just add the rest of X1's bins
    for (; i < X1.size(); ++i) {
      params.emplace_back(X1[i] - X1[i - 1]);
      params.emplace_back(X1[i]);
    }
  } else {
    // In this case we want all of X2's bins first (without the first and last
    // boundaries)
    for (size_t j = 1; j < X2.size() - 1; ++j) {
      params.emplace_back(X2[j] - params.back());
      params.emplace_back(X2[j]);
    }
    // And now those from X1 that lie above the overlap region
    i += overlapbins1;
    for (; i < X1.size(); ++i) {
      params.emplace_back(X1[i] - params.back());
      params.emplace_back(X1[i]);
    }
  }
}

//------------------------------------------------------------------------------------------------
/** Calls the Rebin algorithm as a ChildAlgorithm.
 *  @param  workspace The workspace to use as input to the Rebin algorithms
 *  @param  params    The rebin parameters
 *  @return A shared pointer to the output (rebinned) workspace
 *  @throw  std::runtime_error If the Rebin algorithm fails
 */
API::MatrixWorkspace_sptr MergeRuns::rebinInput(const API::MatrixWorkspace_sptr &workspace,
                                                const std::vector<double> &params) {
  // Create a Rebin child algorithm
  auto rebin = createChildAlgorithm("Rebin");
  rebin->setProperty("InputWorkspace", workspace);
  rebin->setProperty("Params", params);
  rebin->executeAsChildAlg();
  return rebin->getProperty("OutputWorkspace");
}

/** Overriden fillHistory method to correctly store history from merged
 * workspaces.
 */
void MergeRuns::fillHistory() {
  // check if we were merging event or matrix workspaces
  if (!m_inEventWS.empty()) {
    copyHistoryFromInputWorkspaces<std::vector<EventWorkspace_sptr>>(m_inEventWS);
  } else {
    copyHistoryFromInputWorkspaces<std::list<MatrixWorkspace_sptr>>(m_inMatrixWS);
  }
}

/*
 * Here we build up the correct time indexes for the workspace being added. If
 *the scan times for the addee workspace and output workspace are the same this
 *builds the same indexing as the workspace had before. Otherwise, the correct
 *time indexes are set here.
 *
 *This function translates time indices from the addee to the new workspace.
 */
std::vector<SpectrumDefinition> MergeRuns::buildScanIntervals(const std::vector<SpectrumDefinition> &addeeSpecDefs,
                                                              const DetectorInfo &addeeDetInfo,
                                                              const DetectorInfo &newOutDetInfo) {
  std::vector<SpectrumDefinition> newAddeeSpecDefs(addeeSpecDefs.size());

  auto addeeScanIntervals = addeeDetInfo.scanIntervals();
  auto newOutScanIntervals = newOutDetInfo.scanIntervals();

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < int64_t(addeeSpecDefs.size()); ++i) {
    for (auto &index : addeeSpecDefs[i]) {
      SpectrumDefinition newSpecDef;
      for (size_t time_index = 0; time_index < newOutDetInfo.scanCount(); time_index++) {
        if (addeeScanIntervals[index.second] == newOutScanIntervals[time_index]) {
          newSpecDef.add(index.first, time_index);
        }
      }
      newAddeeSpecDefs[i] = newSpecDef;
    }
  }

  return newAddeeSpecDefs;
}

} // namespace Mantid::Algorithms
