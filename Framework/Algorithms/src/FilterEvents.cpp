// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/FilterEvents.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"
#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/DateTimeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include <MantidKernel/InvisibleProperty.h>

#include <memory>
#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Geometry;
using Types::Core::DateAndTime;

using namespace std;

namespace Mantid::Algorithms {

DECLARE_ALGORITHM(FilterEvents)

/** Constructor
 */
FilterEvents::FilterEvents()
    : m_eventWS(), m_splittersWorkspace(), m_splitterTableWorkspace(), m_matrixSplitterWS(), m_detCorrectWorkspace(),
      m_targetWorkspaceIndexSet(), m_outputWorkspacesMap(), m_wsNames(), m_detTofOffsets(), m_detTofFactors(),
      m_filterByPulseTime(false), m_informationWS(), m_hasInfoWS(), m_progress(0.), m_outputWSNameBase(),
      m_toGroupWS(false), m_vecSplitterTime(), m_vecSplitterGroup(), m_tofCorrType(NoneCorrect), m_specSkipType(),
      m_vecSkip(), m_isSplittersRelativeTime(false), m_filterStartTime(0) {}

/** Declare Inputs
 */
void FilterEvents::init() {

  //********************
  // Input Workspaces
  //********************
  std::string titleInputWksp("Input Workspaces");

  declareProperty(std::make_unique<API::WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Input event workspace.");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("SplitterWorkspace", "", Direction::Input),
      "Input workspace specifying \"splitters\", i.e. time intervals and targets for InputWorkspace filtering.");

  declareProperty(
      "RelativeTime", false,
      "Flag indicating whether in SplitterWorkspace the times are absolute or "
      "relative. If true, they are relative to either the run start time or, if specified, FilterStartTime.");
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InformationWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Optional information on each filtering target workspace.");
  setPropertyGroup("InputWorkspace", titleInputWksp);
  setPropertyGroup("SplitterWorkspace", titleInputWksp);
  setPropertyGroup("RelativeTime", titleInputWksp);
  setPropertyGroup("InformationWorkspace", titleInputWksp);

  //********************
  // Output Workspaces
  //********************
  std::string titleOutputWksp("Output Workspaces");

  declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                  "The base name to use for the output workspaces. An output workspace "
                  "name is a combination of this and the target specified in SplitterWorkspace. "
                  "For unfiltered events the workspace name will end with \"_unfiltered\".");

  declareProperty("DescriptiveOutputNames", false,
                  "If selected, the names of the output workspaces will include their time intervals.");

  declareProperty("GroupWorkspaces", false,
                  "An option to group all the output workspaces. The group name will be OutputWorkspaceBaseName.");

  declareProperty("OutputWorkspaceIndexedFrom1", false,
                  "If selected, the target index included in the output workspace name will be 1-based.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputTOFCorrectionWorkspace", "TOFCorrectWS",
                                                                       Direction::Output),
                  "Name of the output workspace for TOF correction factor.");
  setPropertyGroup("OutputWorkspaceBaseName", titleOutputWksp);
  setPropertyGroup("DescriptiveOutputNames", titleOutputWksp);
  setPropertyGroup("GroupWorkspaces", titleOutputWksp);
  setPropertyGroup("OutputWorkspaceIndexedFrom1", titleOutputWksp);
  setPropertyGroup("OutputTOFCorrectionWorkspace", titleOutputWksp);

  //************************
  // Sample logs splitting
  //************************
  std::string titleLogSplit("Sample Logs Splitting");

  // deprecated property kept for backwards compatibility but will not show up in the algorithm's dialog
  declareProperty("SplitSampleLogs", true, "DEPRECATED. All logs will be split.");
  setPropertySettings("SplitSampleLogs", std::make_unique<InvisibleProperty>());

  declareProperty("FilterByPulseTime", false,
                  "If selected, events will be filtered by their pulse time as opposed to full time.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("TimeSeriesPropertyLogs"),
                  "DEPRECATED. All logs will be split.");
  setPropertySettings("TimeSeriesPropertyLogs", std::make_unique<InvisibleProperty>());

  declareProperty("ExcludeSpecifiedLogs", true, "DEPRECATED. All logs will be split.");
  setPropertySettings("ExcludeSpecifiedLogs", std::make_unique<InvisibleProperty>());

  setPropertyGroup("SplitSampleLogs", titleLogSplit);
  setPropertyGroup("FilterByPulseTime", titleLogSplit);
  setPropertyGroup("TimeSeriesPropertyLogs", titleLogSplit);
  setPropertyGroup("ExcludeSpecifiedLogs", titleLogSplit);

  //*********************
  // Additional Options
  //*********************
  std::string titleAdditionalOptions("Additional Options");

  auto dateTime = std::make_shared<DateTimeValidator>();
  dateTime->allowEmpty(true);
  std::string absoluteHelp("Specify date and UTC time in ISO8601 format, e.g. 2010-09-14T04:20:12.");
  declareProperty("FilterStartTime", "", dateTime,
                  "Absolute base time for relative times in SplitterWorkspace. " + absoluteHelp);

  vector<string> corrtypes{"None", "Customized", "Direct", "Elastic", "Indirect"};
  declareProperty("CorrectionToSample", "None", std::make_shared<StringListValidator>(corrtypes),
                  "Type of correction on neutron events to sample time from "
                  "detector time. ");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("DetectorTOFCorrectionWorkspace", "",
                                                                      Direction::Input, PropertyMode::Optional),
                  "Workspace containing the log time correction factor for each detector. ");
  setPropertySettings("DetectorTOFCorrectionWorkspace",
                      std::make_unique<VisibleWhenProperty>("CorrectionToSample", IS_EQUAL_TO, "Customized"));

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
                  "Value of incident energy (Ei) in meV in direct mode.");
  setPropertySettings("IncidentEnergy",
                      std::make_unique<VisibleWhenProperty>("CorrectionToSample", IS_EQUAL_TO, "Direct"));

  // Algorithm to spectra without detectors
  vector<string> spec_no_det{"Skip", "Skip only if TOF correction"};
  declareProperty("SpectrumWithoutDetector", "Skip", std::make_shared<StringListValidator>(spec_no_det),
                  "Approach to deal with spectrum without detectors. ");

  declareProperty("DBSpectrum", EMPTY_INT(), "Spectrum (workspace index) for debug purpose. ");

  setPropertyGroup("FilterStartTime", titleAdditionalOptions);
  setPropertyGroup("CorrectionToSample", titleAdditionalOptions);
  setPropertyGroup("DetectorTOFCorrectionWorkspace", titleAdditionalOptions);
  setPropertyGroup("IncidentEnergy", titleAdditionalOptions);
  setPropertyGroup("SpectrumWithoutDetector", titleAdditionalOptions);
  setPropertyGroup("DBSpectrum", titleAdditionalOptions);

  //*********************
  // Output Properties
  //*********************
  declareProperty("NumberOutputWS", 0, "Number of output workspaces after splitting InputWorkspace. ",
                  Direction::Output);

  declareProperty(std::make_unique<ArrayProperty<string>>("OutputWorkspaceNames", Direction::Output),
                  "List of output workspace names.");

  declareProperty("OutputUnfilteredEvents", false, "If selected, unfiltered events will be output.");
  setPropertyGroup("OutputUnfilteredEvents", titleOutputWksp);
}

std::map<std::string, std::string> FilterEvents::validateInputs() {
  const std::string SPLITTER_PROP_NAME = "SplitterWorkspace";
  std::map<std::string, std::string> result;

  // check the splitters workspace for special behavior
  API::Workspace_const_sptr splitter = this->getProperty(SPLITTER_PROP_NAME);
  // SplittersWorkspace is a special type that needs no further checking
  if (bool(std::dynamic_pointer_cast<const SplittersWorkspace>(splitter))) {
    if (std::dynamic_pointer_cast<const SplittersWorkspace>(splitter)->rowCount() == 0)
      result[SPLITTER_PROP_NAME] = "SplittersWorkspace must have rows defined";
  } else {
    const auto table = std::dynamic_pointer_cast<const TableWorkspace>(splitter);
    const auto matrix = std::dynamic_pointer_cast<const MatrixWorkspace>(splitter);
    if (bool(table)) {
      if (table->columnCount() != 3)
        result[SPLITTER_PROP_NAME] = "TableWorkspace must have 3 columns";
      else if (table->rowCount() == 0)
        result[SPLITTER_PROP_NAME] = "TableWorkspace must have rows defined";
    } else if (bool(matrix)) {
      if (matrix->getNumberHistograms() == 1) {
        if (!matrix->isHistogramData())
          result[SPLITTER_PROP_NAME] = "MatrixWorkspace must be histogram";
      } else {
        result[SPLITTER_PROP_NAME] = "MatrixWorkspace can have only one histogram";
      }
    } else {
      result[SPLITTER_PROP_NAME] = "Incompatible workspace type";
    }
  }

  const string correctiontype = getPropertyValue("CorrectionToSample");
  if (correctiontype == "Direct") {
    double ei = getProperty("IncidentEnergy");
    if (isEmpty(ei)) {
      EventWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");
      if (!inputWS->run().hasProperty("Ei")) {
        const string msg("InputWorkspace does not have Ei. Must specify IncidentEnergy");
        result["CorrectionToSample"] = msg;
        result["IncidentEnergy"] = msg;
      }
    }
  } else if (correctiontype == "Customized") {
    TableWorkspace_const_sptr correctionWS = getProperty("DetectorTOFCorrectionWorkspace");
    if (!correctionWS) {
      const string msg("Must specify correction workspace with CorrectionToSample=Custom");
      result["CorrectionToSample"] = msg;
      result["DetectorTOFCorrectionWorkspace"] = msg;
    }
  }
  // "None" and "Elastic" and "Indirect" don't require extra information

  return result;
}

/** Execution body
 */
void FilterEvents::exec() {
  // Process algorithm properties
  processAlgorithmProperties();

  // Examine workspace for detectors
  examineEventWS();

  // Parse splitters
  m_progress = 0.0;
  progress(m_progress, "Processing input splitters.");
  parseInputSplitters();

  // Create output workspaces
  m_progress = 0.1;
  progress(m_progress, "Create Output Workspaces.");
  createOutputWorkspaces();

  // Optional import corrections
  m_progress = 0.20;
  progress(m_progress, "Importing TOF corrections. ");

  // populate the output workspace associated to the algorithm's property "OutputTOFCorrectionWorkspace"
  // also initialize m_detTofOffsets and m_detTofFactors
  setupDetectorTOFCalibration();

  // Filter Events
  m_progress = 0.30;
  progress(m_progress, "Filter Events.");
  double progressamount;
  if (m_toGroupWS)
    progressamount = 0.6;
  else
    progressamount = 0.7;

  // sort the input events here so tbb can better parallelize the tasks
  {
    const auto startTime = std::chrono::high_resolution_clock::now();
    const auto sortType = m_filterByPulseTime ? EventSortType::PULSETIME_SORT : EventSortType::PULSETIMETOF_SORT;
    m_eventWS->sortAll(sortType, nullptr);
    addTimer("sortEvents", startTime, std::chrono::high_resolution_clock::now());
  }

  // filter the events
  filterEvents(progressamount);

  // Optional to group detector
  groupOutputWorkspace();

  // Set goniometer to output workspaces
  Goniometer gon = m_eventWS->run().getGoniometer();
  for (auto &it : m_outputWorkspacesMap) {
    try {
      DataObjects::EventWorkspace_sptr ws_i = it.second;
      ws_i->mutableRun().setGoniometer(gon, true);
    } catch (std::runtime_error &) {
      g_log.warning("Cannot set goniometer to output workspace.");
    }
  }

  // Set OutputWorkspaceNames property
  std::vector<std::string> outputwsnames;
  for (auto &it : m_outputWorkspacesMap) {
    std::string ws_name = it.second->getName();
    // If OutputUnfilteredEvents is false, the workspace created for unfiltered events has no name.
    // Do not include an empty name into the list of output workspace names.
    if (!ws_name.empty())
      outputwsnames.emplace_back(ws_name);
  }
  setProperty("OutputWorkspaceNames", outputwsnames);

  m_progress = 1.0;
  progress(m_progress, "Completed");
}

//----------------------------------------------------------------------------------------------
/**
 * Mark event lists of workspace indexes with no associated detector pixels as not to be split
 */
void FilterEvents::examineEventWS() {
  size_t numhist = m_eventWS->getNumberHistograms();
  m_vecSkip.resize(numhist, false);

  // check whether any detector is skipped
  if (m_specSkipType == EventFilterSkipNoDetTOFCorr && m_tofCorrType == NoneCorrect) {
    // No TOF correction and skip spectrum only if TOF correction is required
    g_log.warning("By user's choice, No spectrum will be skipped even if it has no detector.");
  } else {
    // check detectors whether there is any of them that will be skipped
    stringstream msgss;
    size_t numskipspec = 0;
    size_t numeventsskip = 0;

    const auto &spectrumInfo = m_eventWS->spectrumInfo();
    for (size_t i = 0; i < numhist; ++i) {
      if (!spectrumInfo.hasDetectors(i)) {
        m_vecSkip[i] = true;

        ++numskipspec;
        const EventList &elist = m_eventWS->getSpectrum(i);
        numeventsskip += elist.getNumberEvents();
        msgss << i;
        if (numskipspec % 10 == 0)
          msgss << "\n";
        else
          msgss << ",";
      }
    } // ENDFOR

    if (numskipspec > 0) {
      g_log.warning() << "There are " << numskipspec << " spectra that do not have detectors. "
                      << "They will be skipped during filtering. There are total " << numeventsskip
                      << " events in those spectra. \nList of these specta is as below:\n"
                      << msgss.str() << "\n";
    } else {
      g_log.notice("All spectra have detectors.");
    }

  } // END-IF-ELSE

  return;
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void FilterEvents::processAlgorithmProperties() {
  m_eventWS = this->getProperty("InputWorkspace");
  if (!m_eventWS) {
    stringstream errss;
    errss << "Inputworkspace is not event workspace. ";
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  //**************************************************************
  // Find out what type of input workspace encodes the splitters
  // Valid options are:
  //   - SplittersWorkspace
  //   - TableWorkspace
  //   - MatrixWorkspace
  //**************************************************************
  // Process splitting workspace (table or data)
  API::Workspace_sptr tempws = this->getProperty("SplitterWorkspace");

  m_splittersWorkspace = std::dynamic_pointer_cast<SplittersWorkspace>(tempws);
  m_splitterTableWorkspace = std::dynamic_pointer_cast<TableWorkspace>(tempws);
  m_matrixSplitterWS = std::dynamic_pointer_cast<MatrixWorkspace>(tempws);

  if (!m_splittersWorkspace && !m_splitterTableWorkspace && !m_matrixSplitterWS)
    throw runtime_error("Input \"SplitterWorkspace\" has invalid workspace type.");

  // Does the splitter workspace contains absolute or relative times?
  m_isSplittersRelativeTime = this->getProperty("RelativeTime");

  //********************************************************
  // Find out if an InformationWorkspace has been supplied
  //********************************************************
  m_informationWS = this->getProperty("InformationWorkspace");
  // Information workspace is specified?
  if (!m_informationWS)
    m_hasInfoWS = false;
  else
    m_hasInfoWS = true;

  m_filterByPulseTime = this->getProperty("FilterByPulseTime");

  //***************************************
  // Information on the Ouptut Workspaces
  //   - OutputWorkspaceBaseName
  //   - GroupWorkspaces
  //***************************************
  m_outputWSNameBase = this->getPropertyValue("OutputWorkspaceBaseName");
  m_toGroupWS = this->getProperty("GroupWorkspaces");
  if (m_toGroupWS && (m_outputWSNameBase == m_eventWS->getName())) {
    std::stringstream errss;
    errss << "It is not allowed to group output workspaces into the same name "
             "(i..e, OutputWorkspaceBaseName = "
          << m_outputWSNameBase << ") as the input workspace to filter events from.";
    throw std::invalid_argument(errss.str());
  }

  //-------------------------------------------------------------------------
  // TOF detector/sample correction
  //-------------------------------------------------------------------------
  // Type of correction
  string correctiontype = getPropertyValue("CorrectionToSample");
  if (correctiontype == "None")
    m_tofCorrType = NoneCorrect;
  else if (correctiontype == "Customized")
    m_tofCorrType = CustomizedCorrect;
  else if (correctiontype == "Direct")
    m_tofCorrType = DirectCorrect;
  else if (correctiontype == "Elastic")
    m_tofCorrType = ElasticCorrect;
  else if (correctiontype == "Indirect")
    m_tofCorrType = IndirectCorrect;
  else {
    g_log.error() << "Correction type '" << correctiontype << "' is not supported. \n";
    throw runtime_error("Impossible situation!");
  }

  // Spectrum skip
  string skipappr = getPropertyValue("SpectrumWithoutDetector");
  if (skipappr == "Skip")
    m_specSkipType = EventFilterSkipNoDet;
  else if (skipappr == "Skip only if TOF correction")
    m_specSkipType = EventFilterSkipNoDetTOFCorr;
  else
    throw runtime_error("An unrecognized option for SpectrumWithoutDetector");

  if (!isDefault("FilterStartTime")) {
    std::string filter_start_time_str =
        getProperty("FilterStartTime"); // User-specified absolute base time for relative times
    m_filterStartTime = Types::Core::DateAndTime(filter_start_time_str);
  } else {
    // Default filter starting time to the run start time
    try {
      m_filterStartTime = m_eventWS->run().startTime();
    } catch (std::runtime_error &) {
      throw std::runtime_error(
          "InputWorkspace doesn't have valid run start time set, and FilterStartTime is not specified.");
    }
  }

  //-------------------------------------------------------------------------
  // Deprecated properties
  //-------------------------------------------------------------------------
  bool splitSampleLogs = getProperty("SplitSampleLogs");
  if (splitSampleLogs == false)
    g_log.warning() << "Option SplitSampleLogs is deprecated and ignored. All logs will be split.\n";

  std::vector<std::string> timeSeriesPropertyLogs = getProperty("TimeSeriesPropertyLogs");
  if (!timeSeriesPropertyLogs.empty())
    g_log.warning() << "Options TimeSeriesPropertyLogs and ExcludeSpecifiedLogs are deprecated and ignored. "
                       "All logs will be split.\n";
}

//----------------------------------------------------------------------------------------------
/** group output workspaces
 */
void FilterEvents::groupOutputWorkspace() {
  // return if there is no such need
  if (!m_toGroupWS)
    return;
  // set progress
  m_progress = 0.95;
  progress(m_progress, "Group workspaces");

  std::string groupname = m_outputWSNameBase;
  auto groupws = createChildAlgorithm("GroupWorkspaces", 0.95, 1.00, true);
  groupws->setAlwaysStoreInADS(true);
  groupws->setProperty("InputWorkspaces", m_wsNames);
  groupws->setProperty("OutputWorkspace", groupname);
  groupws->execute();
  if (!groupws->isExecuted()) {
    g_log.error("Grouping all output workspaces fails.");
    return;
  }

  // set the group workspace as output workspace
  if (!this->existsProperty("OutputWorkspace")) {
    declareProperty(
        std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", groupname, Direction::Output),
        "Name of the workspace to be created as the output of grouping ");
  }

  const AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
  API::WorkspaceGroup_sptr workspace_group = std::dynamic_pointer_cast<WorkspaceGroup>(ads.retrieve(groupname));
  if (!workspace_group) {
    g_log.error("Unable to retrieve output workspace from algorithm GroupWorkspaces");
    return;
  }
  setProperty("OutputWorkspace", workspace_group);

  return;
}

/**
 * Find the TimeROI associated to the current destination-workspace index.
 *
 * Caveat: when the destination workspace is the unfiltered workspace, we have the unfortunate coincidence that
 * its index, TimeSplitter::NO_TARGET, is the same as the value denoting the end of a splitting.
 * Illuminating example:
 * TimeSplitter(0.0, 1.0, 1) results in TimeSplitter::m_roi_map = {(0.0, 1), (1.0, NO_TARGET)}, but
 * TimeSplitter(0.0, 1.0, NO_TARGET) is ill-defined, as {(0.0, NO_TARGET), (1.0, NO_TARGET)} is meaningless.
 */
TimeROI FilterEvents::partialROI(const int &index) {
  TimeROI roi{m_timeSplitter.getTimeROI(index)};

  if (index == TimeSplitter::NO_TARGET) {
    const auto splittingBoundaries = m_timeSplitter.getSplittersMap();

    const auto firstSplitter = splittingBoundaries.begin();
    // add leading mask as ROI of the unfiltered workspace
    if (firstSplitter->first > m_filterStartTime)
      roi.addROI(m_filterStartTime, firstSplitter->first);

    const auto lastSplitter = std::prev(splittingBoundaries.end());
    // sanity check
    if (lastSplitter->second != TimeSplitter::NO_TARGET)
      throw std::runtime_error("FilterEvents::partialROI: the last splitter boundary must be TimeSplitter::NO_TARGET");
    // add trailing mask as ROI of the unfiltered workspace
    if (lastSplitter->first < m_eventWS->run().endTime())
      roi.addROI(lastSplitter->first, m_eventWS->run().endTime());
  }

  return roi;
}

/** Convert splitters specified by the input workspace to TimeSplitter
 *  Cache a set of all target workspace indexes
 * @brief FilterEvents::parseInputSplitters
 */
void FilterEvents::parseInputSplitters() {
  if (m_splittersWorkspace) {
    m_timeSplitter = TimeSplitter{m_splittersWorkspace};
  } else if (m_splitterTableWorkspace) {
    m_timeSplitter =
        TimeSplitter(m_splitterTableWorkspace, m_isSplittersRelativeTime ? m_filterStartTime : DateAndTime::GPS_EPOCH);
  } else {
    m_timeSplitter =
        TimeSplitter(m_matrixSplitterWS, m_isSplittersRelativeTime ? m_filterStartTime : DateAndTime::GPS_EPOCH);
  }

  m_targetWorkspaceIndexSet = m_timeSplitter.outputWorkspaceIndices();

  // For completeness, make sure we have a special workspace index for unfiltered events
  m_targetWorkspaceIndexSet.insert(TimeSplitter::NO_TARGET);
}

//-------------------------------------------------------------------------
/** Create a list of EventWorkspace objects to be used as event filtering output.
 * Sets the TimeROI for each destination workspace
 */
void FilterEvents::createOutputWorkspaces() {
  const auto startTimeCreateWS = std::chrono::high_resolution_clock::now();

  // There is always NO_TARGET index included in the set, plus we need at least one "valid target" index
  constexpr size_t min_expected_number_of_indexes = 2;
  if (m_targetWorkspaceIndexSet.size() < min_expected_number_of_indexes) {
    g_log.warning("No output workspaces specified by input workspace.");
    return;
  }

  // Convert information workspace to map
  std::map<int, std::string> infomap;
  if (m_hasInfoWS) {
    // Check information workspace consistency
    if (m_targetWorkspaceIndexSet.size() - 1 != m_informationWS->rowCount()) {
      g_log.warning() << "Input Splitters Workspace specifies a different number of unique output workspaces ("
                      << m_targetWorkspaceIndexSet.size() - 1
                      << ") compared to the number of rows in the input information workspace ("
                      << m_informationWS->rowCount() << "). "
                      << "  Information may not be accurate. \n";
    }

    for (size_t ir = 0; ir < m_informationWS->rowCount(); ++ir) {
      API::TableRow row = m_informationWS->getRow(ir);
      infomap.emplace(row.Int(0), row.String(1));
    }
  }

  // See if we need to count the valid target indexes from 1. That will only affect the names of the output workspaces.
  int delta_wsindex{0}; // an index shift
  if (getProperty("OutputWorkspaceIndexedFrom1")) {
    // Determine the minimum valid target index. Note that the set is sorted and guaranteed to start with -1.
    const int min_wsindex = m_targetWorkspaceIndexSet.size() == 1 ? *m_targetWorkspaceIndexSet.begin()
                                                                  : *std::next(m_targetWorkspaceIndexSet.begin(), 1);
    g_log.debug() << "Minimum target workspace index = " << min_wsindex << "\n";
    delta_wsindex = 1 - min_wsindex;
  }

  // Work out how it has been split so the naming can be done
  // if generateEventsFilter has been used the infoWS will contain time or log
  // otherwise it is assumed that it has been split by time.
  bool descriptiveNames = getProperty("DescriptiveOutputNames");
  bool splitByTime = true;
  if (descriptiveNames) {
    if (m_hasInfoWS && infomap[0].find("Log") != std::string::npos) {
      splitByTime = false;
    }
  }

  // Clone the input workspace but without any events. Will serve as blueprint for the output workspaces
  std::shared_ptr<EventWorkspace> templateWorkspace = create<EventWorkspace>(*m_eventWS);
  templateWorkspace->setSharedRun(Kernel::make_cow<Run>()); // clear the run object
  templateWorkspace->clearMRU();
  templateWorkspace->switchEventType(m_eventWS->getEventType());

  // Set up target workspaces
  size_t number_of_output_workspaces{0};
  double progress_step_total =
      static_cast<double>(m_targetWorkspaceIndexSet.size()); // total number of progress steps expected
  double progress_step_current{0.};                          // current number of progress steps
  const auto originalROI = m_eventWS->run().getTimeROI();
  const bool outputUnfiltered = getProperty("OutputUnfilteredEvents");
  for (auto const wsindex : m_targetWorkspaceIndexSet) {
    // Generate new workspace name
    bool add2output = true;
    std::stringstream wsname;
    wsname << m_outputWSNameBase << "_";
    if (wsindex > TimeSplitter::NO_TARGET) {
      if (descriptiveNames && splitByTime) {
        TimeROI timeROI = m_timeSplitter.getTimeROI(wsindex);
        auto timeIntervals = timeROI.toTimeIntervals();
        for (size_t ii = 0; ii < timeIntervals.size(); ii++) {
          auto startTimeInSeconds =
              Mantid::Types::Core::DateAndTime::secondsFromDuration(timeIntervals[ii].start() - m_filterStartTime);
          auto stopTimeInSeconds =
              Mantid::Types::Core::DateAndTime::secondsFromDuration(timeIntervals[ii].stop() - m_filterStartTime);
          wsname << startTimeInSeconds << "_" << stopTimeInSeconds;
          if (ii < timeIntervals.size() - 1)
            wsname << "_";
        }
      } else if (descriptiveNames) {
        auto infoiter = infomap.find(wsindex);
        if (infoiter != infomap.end()) {
          std::string nameFromMap = infoiter->second;
          nameFromMap = Kernel::Strings::removeSpace(nameFromMap);
          wsname << nameFromMap;
        } else {
          wsname << m_timeSplitter.getWorkspaceIndexName(wsindex, delta_wsindex);
        }
      } else {
        wsname << m_timeSplitter.getWorkspaceIndexName(wsindex, delta_wsindex);
      }
    } else {
      wsname << "unfiltered";
      if (!outputUnfiltered)
        add2output = false;
    }

    //
    // instantiate one of the output filtered workspaces
    std::shared_ptr<EventWorkspace> optws = templateWorkspace->clone();

    m_outputWorkspacesMap.emplace(wsindex, optws);

    //
    // Add information, including title and comment, to output workspace
    if (m_hasInfoWS) {
      std::string info;
      if (wsindex < 0) {
        info = "Events that are filtered out. ";
      } else {
        std::map<int, std::string>::iterator infoiter;
        infoiter = infomap.find(wsindex);
        if (infoiter != infomap.end()) {
          info = infoiter->second;
        } else {
          info = "This workspace has no information provided. ";
        }
      }
      optws->setComment(info);
      optws->setTitle(info);
    }

    //
    // Declare the filtered workspace as an output property.
    // There shouldn't be any non-unfiltered workspace skipped from group index
    if (add2output) {
      // Generate the name of the output property. Only workspaces that are
      // set as output properties get history added to them
      std::stringstream outputWorkspacePropertyName;
      if (wsindex == TimeSplitter::NO_TARGET) {
        outputWorkspacePropertyName << "OutputWorkspace_unfiltered";
      } else {
        outputWorkspacePropertyName << "OutputWorkspace_" << wsindex;
      }

      // Inserted this pair to map
      m_wsNames.emplace_back(wsname.str());

      // Set (property) to output workspace and set to ADS
      AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

      // create and initialize the property for the current output workspace
      if (!m_toGroupWS) {
        if (!this->existsProperty(outputWorkspacePropertyName.str())) {
          declareProperty(std::make_unique<API::WorkspaceProperty<DataObjects::EventWorkspace>>(
                              outputWorkspacePropertyName.str(), wsname.str(), Direction::Output),
                          "Output Workspace");
        }
        setProperty(outputWorkspacePropertyName.str(), optws);
        g_log.debug() << "Created output property " << outputWorkspacePropertyName.str()
                      << " for workspace with target index " << wsindex << std::endl;
      }

      ++number_of_output_workspaces;
      g_log.debug() << "Created output Workspace with target index = " << wsindex << std::endl;

      // Update progress report
      m_progress = 0.1 + 0.1 * progress_step_current / progress_step_total;
      progress(m_progress, "Creating output workspace");
      progress_step_current += 1.;
    }

  } // end of the loop iterating over the elements of m_targetWorkspaceIndexSet
  addTimer("createOutputWorkspaces", startTimeCreateWS, std::chrono::high_resolution_clock::now());

  // drop shared pointer for template
  templateWorkspace.reset();

  // copy the logs over
  const auto startTimeLogs = std::chrono::high_resolution_clock::now();
  for (auto &outputIter : m_outputWorkspacesMap) {
    // Endow the output workspace with a TimeROI
    TimeROI roi = this->partialROI(outputIter.first);
    roi.update_or_replace_intersection(originalROI);

    // discard log entries outside the ROI
    outputIter.second->mutableRun().copyAndFilterProperties(m_eventWS->run(), roi);
  }
  addTimer("copyLogs", startTimeLogs, std::chrono::high_resolution_clock::now());

  setProperty("NumberOutputWS", static_cast<int>(number_of_output_workspaces));

  g_log.information("Output workspaces are created. ");

} // END OF FilterEvents::createOutputWorkspaces()

/** Set up neutron event's TOF correction.
 * It can be (1) parsed from TOF-correction table workspace to vectors,
 * (2) created according to detector's position in instrument;
 * (3) or no correction,i.e., correction value is equal to 1.
 * Offset should be as F*TOF + B
 */
void FilterEvents::setupDetectorTOFCalibration() {
  // Set output correction workspace and set to output
  const size_t numhist = m_eventWS->getNumberHistograms();
  MatrixWorkspace_sptr corrws = create<Workspace2D>(numhist, Points(2));
  setProperty("OutputTOFCorrectionWorkspace", corrws);

  // Set up the size of correction and output correction workspace
  m_detTofOffsets.resize(numhist, 0.0); // unit of TOF
  m_detTofFactors.resize(numhist, 1.0); // multiplication factor

  // Set up detector values
  std::unique_ptr<TimeAtSampleStrategy> strategy;

  if (m_tofCorrType == CustomizedCorrect) {
    setupCustomizedTOFCorrection();
  } else if (m_tofCorrType == ElasticCorrect) {
    // Generate TOF correction from instrument's set up
    strategy.reset(setupElasticTOFCorrection());
  } else if (m_tofCorrType == DirectCorrect) {
    // Generate TOF correction for direct inelastic instrument
    strategy.reset(setupDirectTOFCorrection());
  } else if (m_tofCorrType == IndirectCorrect) {
    // Generate TOF correction for indirect elastic instrument
    strategy.reset(setupIndirectTOFCorrection());
  }
  if (strategy) {
    for (size_t i = 0; i < numhist; ++i) {
      if (!m_vecSkip[i]) {

        Correction correction = strategy->calculate(i);
        m_detTofOffsets[i] = correction.offset;
        m_detTofFactors[i] = correction.factor;

        corrws->mutableY(i)[0] = correction.factor;
        corrws->mutableY(i)[1] = correction.offset;
      }
    }
  }
}

TimeAtSampleStrategy *FilterEvents::setupElasticTOFCorrection() const {

  return new TimeAtSampleStrategyElastic(m_eventWS);
}

TimeAtSampleStrategy *FilterEvents::setupDirectTOFCorrection() const {

  // Get incident energy Ei
  double ei = getProperty("IncidentEnergy");
  if (isEmpty(ei)) {
    if (m_eventWS->run().hasProperty("Ei")) {
      ei = m_eventWS->run().getLogAsSingleValue("Ei");
      g_log.debug() << "Using stored Ei value " << ei << "\n";
    } else {
      throw std::invalid_argument("No Ei value has been set or stored within the run information.");
    }
  } else {
    g_log.debug() << "Using user-input Ei value " << ei << "\n";
  }

  return new TimeAtSampleStrategyDirect(m_eventWS, ei);
}

TimeAtSampleStrategy *FilterEvents::setupIndirectTOFCorrection() const {
  return new TimeAtSampleStrategyIndirect(m_eventWS);
}

/** Set up corrections with customized TOF correction input
 * The first column must be either DetectorID or Spectrum (from 0... as
 * workspace index)
 * The second column must be Correction or CorrectFactor, a number between 0
 * and 1, i.e, [0, 1]
 * The third column is optional as shift in unit of second
 */
void FilterEvents::setupCustomizedTOFCorrection() {
  // Check input workspace
  m_detCorrectWorkspace = getProperty("DetectorTOFCorrectionWorkspace");
  vector<string> colnames = m_detCorrectWorkspace->getColumnNames();
  bool hasshift = false;
  if (colnames.size() < 2)
    throw runtime_error("Input table workspace is not valide.");
  else if (colnames.size() >= 3)
    hasshift = true;

  bool usedetid;
  if (colnames[0] == "DetectorID")
    usedetid = true;
  else if (colnames[0] == "Spectrum")
    usedetid = false;
  else {
    stringstream errss;
    errss << "First column must be either DetectorID or Spectrum. " << colnames[0] << " is not supported. ";
    throw runtime_error(errss.str());
  }

  // Parse detector and its TOF correction (i.e., offset factor and tof shift)
  // to a map
  map<detid_t, double> toffactormap;
  map<detid_t, double> tofshiftmap;
  size_t numrows = m_detCorrectWorkspace->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
    TableRow row = m_detCorrectWorkspace->getRow(i);

    // Parse to map
    detid_t detid;
    double offset_factor;
    row >> detid >> offset_factor;
    if (offset_factor >= 0 && offset_factor <= 1) {
      // Valid offset (factor value)
      toffactormap.emplace(detid, offset_factor);
    } else {
      // Error, throw!
      stringstream errss;
      errss << "Correction (i.e., offset) equal to " << offset_factor << " of row "
            << "is out of range [0, 1].";
      throw runtime_error(errss.str());
    }

    // Shift
    if (hasshift) {
      double shift;
      row >> shift;
      tofshiftmap.emplace(detid, shift);
    }
  } // ENDFOR(row i)

  // Check size of TOF correction map
  size_t numhist = m_eventWS->getNumberHistograms();
  if (toffactormap.size() > numhist) {
    g_log.warning() << "Input correction table workspace has more detectors (" << toffactormap.size()
                    << ") than input workspace " << m_eventWS->getName() << "'s spectra number (" << numhist << ".\n";
  } else if (toffactormap.size() < numhist) {
    stringstream errss;
    errss << "Input correction table workspace has more detectors (" << toffactormap.size() << ") than input workspace "
          << m_eventWS->getName() << "'s spectra number (" << numhist << ".\n";
    throw runtime_error(errss.str());
  }

  // Apply to m_detTofOffsets and m_detTofShifts
  if (usedetid) {
    // Get vector IDs
    vector<detid_t> vecDetIDs(numhist, 0);
    // Set up the detector IDs to vecDetIDs and set up the initial value
    for (size_t i = 0; i < numhist; ++i) {
      // It is assumed that there is one detector per spectra.
      // If there are more than 1 spectrum, it is very likely to have problem
      // with correction factor
      const DataObjects::EventList events = m_eventWS->getSpectrum(i);
      const auto &detids = events.getDetectorIDs();
      if (detids.size() != 1) {
        // Check whether there are more than 1 detector per spectra.
        stringstream errss;
        errss << "The assumption is that one spectrum has one and only one "
                 "detector. "
              << "Error is found at spectrum " << i << ".  It has " << detids.size() << " detectors.";
        throw runtime_error(errss.str());
      }
      vecDetIDs[i] = *detids.begin();
    }

    // Map correction map to list
    map<detid_t, double>::iterator fiter;
    for (size_t i = 0; i < numhist; ++i) {
      detid_t detid = vecDetIDs[i];
      // correction (factor) map
      fiter = toffactormap.find(detid);
      if (fiter != toffactormap.end())
        m_detTofFactors[i] = fiter->second;
      else {
        stringstream errss;
        errss << "Detector "
              << "w/ ID << " << detid << " of spectrum " << i << " in Eventworkspace " << m_eventWS->getName()
              << " cannot be found in input TOF calibration workspace. ";
        throw runtime_error(errss.str());
      }
      // correction shift map
      fiter = tofshiftmap.find(detid);
      if (fiter != tofshiftmap.end())
        m_detTofOffsets[i] = fiter->second;
    } // ENDFOR (each spectrum i)
  } else {
    // It is spectrum Number already
    map<detid_t, double>::iterator fiter;
    // correction factor
    for (fiter = toffactormap.begin(); fiter != toffactormap.end(); ++fiter) {
      auto wsindex = static_cast<size_t>(fiter->first);
      if (wsindex < numhist)
        m_detTofFactors[wsindex] = fiter->second;
      else {
        stringstream errss;
        errss << "Workspace index " << wsindex << " is out of range.";
        throw runtime_error(errss.str());
      }
    }
    // correction shift
    for (fiter = tofshiftmap.begin(); fiter != tofshiftmap.end(); ++fiter) {
      auto wsindex = static_cast<size_t>(fiter->first);
      if (wsindex < numhist)
        m_detTofOffsets[wsindex] = fiter->second;
      else {
        stringstream errss;
        errss << "Workspace index " << wsindex << " is out of range.";
        throw runtime_error(errss.str());
      }
    }
  }
}

void FilterEvents::filterEvents(double progressamount) {
  const bool pulseTof{!m_filterByPulseTime};           // split by pulse-time + TOF ?
  const bool tofCorrect{m_tofCorrType != NoneCorrect}; // apply corrections to the TOF values?
  const auto startTime = std::chrono::high_resolution_clock::now();
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  g_log.debug() << "Number of spectra in input/source EventWorkspace = " << numberOfSpectra << ".\n";

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws) {
    PARALLEL_START_INTERRUPT_REGION
    if (!m_vecSkip[iws]) {                                                        // Filter the non-skipped
      const DataObjects::EventList &inputEventList = m_eventWS->getSpectrum(iws); // input event list
      if (!inputEventList.empty()) {                              // nothing to split if there aren't events
        std::map<int, DataObjects::EventList *> partialEvenLists; // event lists receiving the events from input list
        for (auto &ws : m_outputWorkspacesMap) {
          const int index = ws.first;
          auto &partialEventList = ws.second->getSpectrum(iws);
          partialEvenLists.emplace(index, &partialEventList);
        }
        m_timeSplitter.splitEventList(inputEventList, partialEvenLists, pulseTof, tofCorrect, m_detTofFactors[iws],
                                      m_detTofOffsets[iws]);
      }
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  progress(0.1 + progressamount, "Splitting logs");
  addTimer("filterEventsMethod", startTime, std::chrono::high_resolution_clock::now());
}

} // namespace Mantid::Algorithms
