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
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"

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
      m_useSplittersWorkspace(false), m_useArbTableSplitters(false), m_targetWorkspaceIndexSet(),
      m_outputWorkspacesMap(), m_wsNames(), m_detTofOffsets(), m_detTofFactors(), m_filterByPulseTime(false),
      m_informationWS(), m_hasInfoWS(), m_progress(0.), m_outputWSNameBase(), m_toGroupWS(false), m_vecSplitterTime(),
      m_vecSplitterGroup(), m_splitSampleLogs(false), m_useDBSpectrum(false), m_dbWSIndex(-1),
      m_tofCorrType(NoneCorrect), m_specSkipType(), m_vecSkip(), m_isSplittersRelativeTime(false), m_filterStartTime(0),
      m_runStartTime(0) {}

/** Declare Inputs
 */
void FilterEvents::init() {

  //********************
  // Input Workspaces
  //********************
  std::string titleInputWksp("Input Workspaces");

  declareProperty(std::make_unique<API::WorkspaceProperty<EventWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>("SplitterWorkspace", "", Direction::Input),
                  "An input SplittersWorskpace for filtering");

  declareProperty("RelativeTime", false,
                  "Flag to indicate that in the input Matrix splitting workspace, the time indicated "
                  "by X-vector is relative to either run start time or some indicted time.");
  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("InformationWorkspace", "", Direction::Input,
                                                                      PropertyMode::Optional),
                  "Optional output for the information of each splitter "
                  "workspace index.");
  setPropertyGroup("InputWorkspace", titleInputWksp);
  setPropertyGroup("SplitterWorkspace", titleInputWksp);
  setPropertyGroup("RelativeTime", titleInputWksp);
  setPropertyGroup("InformationWorkspace", titleInputWksp);

  //********************
  // Output Workspaces
  //********************
  std::string titleOutputWksp("Output Workspaces");

  declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                  "The base name to use for the output workspaces. An output "
                  "workspace name is the base name plus a suffix separated by an underscore. "
                  "The suffix is either a target name (numeric or non-numeric) or \"unfiltered\".");

  declareProperty("DescriptiveOutputNames", false,
                  "If selected, the names of the output workspaces will include information about each slice.");

  declareProperty("GroupWorkspaces", false,
                  "Option to group all the output workspaces.  Group name will be OutputWorkspaceBaseName.");

  declareProperty("OutputWorkspaceIndexedFrom1", false,
                  "If selected, the names of the output workspaces will have suffix indexes starting from 1. "
                  "This applies to numeric target names only.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputTOFCorrectionWorkspace", "TOFCorrectWS",
                                                                       Direction::Output),
                  "Name of output workspace for TOF correction factor. ");
  setPropertyGroup("OutputWorkspaceBaseName", titleOutputWksp);
  setPropertyGroup("DescriptiveOutputNames", titleOutputWksp);
  setPropertyGroup("GroupWorkspaces", titleOutputWksp);
  setPropertyGroup("OutputWorkspaceIndexedFrom1", titleOutputWksp);
  setPropertyGroup("OutputTOFCorrectionWorkspace", titleOutputWksp);

  //************************
  // Sample logs splitting
  //************************
  std::string titleLogSplit("Sample Logs Splitting");
  declareProperty("SplitSampleLogs", true,
                  "If selected, all sample logs will be split by the event splitters.  It is not recommended "
                  "for fast event log splitters.");

  declareProperty("FilterByPulseTime", false,
                  "Filter the event by its pulse time only for slow sample environment log.  This option can make "
                  "execution of the algorithm faster, but lowers its precision.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("TimeSeriesPropertyLogs"),
                  "List of name of sample logs of TimeSeriesProperty format. "
                  "They will be either excluded from splitting if ExcludedSpecifiedLogs is specified as True. "
                  "Alternatively, they will be the only TimeSeriesProperty sample logs that will be split "
                  "to child workspaces.");
  declareProperty("ExcludeSpecifiedLogs", true,
                  "If true, all the TimeSeriesProperty logs listed will be excluded from duplicating. "
                  "Otherwise, only those specified logs will be split.");
  setPropertyGroup("SplitSampleLogs", titleLogSplit);
  setPropertyGroup("FilterByPulseTime", titleLogSplit);
  setPropertyGroup("TimeSeriesPropertyLogs", titleLogSplit);
  setPropertyGroup("ExcludeSpecifiedLogs", titleLogSplit);

  //*********************
  // Additional Options
  //*********************
  std::string titleAdditionalOptions("Additional Options");

  declareProperty("FilterStartTime", "", "Start time for splitters that can be parsed to DateAndTime.");

  vector<string> corrtypes{"None", "Customized", "Direct", "Elastic", "Indirect"};
  declareProperty("CorrectionToSample", "None", std::make_shared<StringListValidator>(corrtypes),
                  "Type of correction on neutron events to sample time from "
                  "detector time. ");

  declareProperty(std::make_unique<WorkspaceProperty<TableWorkspace>>("DetectorTOFCorrectionWorkspace", "",
                                                                      Direction::Input, PropertyMode::Optional),
                  "Name of table workspace containing the log time correction factor for each detector. ");
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
  declareProperty("NumberOutputWS", 0, "Number of output output workspace splitted. ", Direction::Output);

  declareProperty(std::make_unique<ArrayProperty<string>>("OutputWorkspaceNames", Direction::Output),
                  "List of output workspaces names");

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
  // m_progress = 0.05;
  // progress(m_progress);

  // Create output workspaces
  m_progress = 0.1;
  progress(m_progress, "Create Output Workspaces.");
  createOutputWorkspaces();

  // DEBUG: mark these vectors for deletion
  // std::vector<Kernel::TimeSeriesProperty<int> *> int_tsp_vector;
  // std::vector<Kernel::TimeSeriesProperty<double> *> dbl_tsp_vector;
  // std::vector<Kernel::TimeSeriesProperty<bool> *> bool_tsp_vector;
  // std::vector<Kernel::TimeSeriesProperty<string> *> string_tsp_vector;
  // copyNoneSplitLogs(int_tsp_vector, dbl_tsp_vector, bool_tsp_vector, string_tsp_vector);

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

  // add a new 'split' tsp to output workspace
  // DEBUG: mark for deletion
  std::vector<std::unique_ptr<Kernel::TimeSeriesProperty<int>>> split_tsp_vector;
  filterEventsBySplitters(progressamount);
  // DEBUG: mark for deletion
  // generateSplitterTSPalpha(split_tsp_vector);

  // DEBUG: mark for deletion
  // assign split_tsp_vector to all the output workspaces!
  // mapSplitterTSPtoWorkspaces(split_tsp_vector);

  // DEBUG: mark for deletion
  // split times series property: new way to split events
  // splitTimeSeriesLogs(int_tsp_vector, dbl_tsp_vector, bool_tsp_vector, string_tsp_vector);

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
  if (m_splittersWorkspace) {
    m_useSplittersWorkspace = true;
  } else if (m_splitterTableWorkspace)
    m_useArbTableSplitters = true;
  else {
    m_matrixSplitterWS = std::dynamic_pointer_cast<MatrixWorkspace>(tempws);
    if (m_matrixSplitterWS) {
      m_useSplittersWorkspace = false;
    } else {
      throw runtime_error("Invalid type of input workspace, neither SplittersWorkspace nor MatrixWorkspace.");
    }
  }

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
  m_splitSampleLogs = getProperty("SplitSampleLogs");

  // Debug spectrum
  m_dbWSIndex = getProperty("DBSpectrum");
  if (isEmpty(m_dbWSIndex))
    m_useDBSpectrum = false;
  else
    m_useDBSpectrum = true;

  bool start_time_set = false;
  // Get run start time
  try {
    m_runStartTime = m_eventWS->run().startTime();
    start_time_set = true;
  } catch (std::runtime_error &) {
  }

  // Splitters are given relative time
  m_isSplittersRelativeTime = getProperty("RelativeTime");
  if (m_isSplittersRelativeTime) {
    // Using relative time
    std::string start_time_str = getProperty("FilterStartTime");
    if (!start_time_str.empty()) {
      // User specifies the filter starting time
      Types::Core::DateAndTime temp_shift_time(start_time_str);
      m_filterStartTime = temp_shift_time;
    } else {
      // Retrieve filter starting time from property run_start as default
      if (start_time_set) {
        m_filterStartTime = m_runStartTime;
      } else {
        throw std::runtime_error("Input event workspace does not have property run_start. "
                                 "User does not specifiy filter start time."
                                 "Splitters cannot be in reltive time.");
      }
    }
  } // END-IF: m_isSplitterRelativeTime
}

//----------------------------------------------------------------------------------------------
/** group output workspaces
 * @brief FilterEvents::groupOutputWorkspace
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

//----------------------------------------------------------------------------------------------
// /* Clone the sample logs that will not be split, including single-value and add
//  * all the TimeSeriesProperty sample logs  to vectors by their type
//  * @brief FilterEvents::copyNoneSplitLogs
//  * @param int_tsp_name_vector :: output
//  * @param dbl_tsp_name_vector :: output
//  * @param bool_tsp_name_vector :: output
//  * @param string_tsp_vector :: output
//  */
/**
void FilterEvents::copyNoneSplitLogs(std::vector<TimeSeriesProperty<int> *> &int_tsp_name_vector,
                                     std::vector<TimeSeriesProperty<double> *> &dbl_tsp_name_vector,
                                     std::vector<TimeSeriesProperty<bool> *> &bool_tsp_name_vector,
                                     std::vector<Kernel::TimeSeriesProperty<string> *> &string_tsp_vector) {
  // In not empty, this property specifies which logs will either be excluded or kept
  std::vector<std::string> tsp_logs = getProperty("TimeSeriesPropertyLogs");
  // cast the list into a set of relevant logs
  std::set<std::string> tsp_logs_set(tsp_logs.begin(), tsp_logs.end());

  // this property determines if the relevant logs are to be excluded or are to be kept
  bool excludeRelevantLogs = getProperty("ExcludeSpecifiedLogs");
  bool keepOnlyRelevantLogs{!excludeRelevantLogs};

  // initialize
  int_tsp_name_vector.clear();
  dbl_tsp_name_vector.clear();
  bool_tsp_name_vector.clear();

  std::vector<Property *> prop_vector = m_eventWS->run().getProperties();
  for (auto *prop_i : prop_vector) {
    // get property
    std::string name_i = prop_i->name();

    // cast to different type of TimeSeriesProperties to check it the property is a TimeSeriesProperty
    auto *dbl_prop = dynamic_cast<TimeSeriesProperty<double> *>(prop_i);
    auto *int_prop = dynamic_cast<TimeSeriesProperty<int> *>(prop_i);
    auto *bool_prop = dynamic_cast<TimeSeriesProperty<bool> *>(prop_i);
    auto *string_prop = dynamic_cast<TimeSeriesProperty<string> *>(prop_i);

    if (dbl_prop || int_prop || bool_prop || string_prop) {
      bool logIsRelevant{tsp_logs_set.find(name_i) != tsp_logs_set.end()};
      if (excludeRelevantLogs && logIsRelevant) {
        g_log.warning() << "Skip splitting sample log " << name_i << "\n";
        continue;
      } else if (keepOnlyRelevantLogs && !logIsRelevant) {
        g_log.warning() << "Skip splitting sample log " << name_i << "\n";
        continue;
      }
      // insert the time series property into the corresponding target vector of properties
      if (dbl_prop)
        dbl_tsp_name_vector.emplace_back(dbl_prop);
      else if (int_prop)
        int_tsp_name_vector.emplace_back(int_prop);
      else if (bool_prop)
        bool_tsp_name_vector.emplace_back(bool_prop);
      else if (string_prop)
        string_tsp_vector.emplace_back(string_prop);
    }
    else { // non time series properties
      // single value property: copy to the new workspace
      std::map<int, DataObjects::EventWorkspace_sptr>::iterator ws_iter;
      for (ws_iter = m_outputWorkspacesMap.begin(); ws_iter != m_outputWorkspacesMap.end(); ++ws_iter) {
        std::string value_i = prop_i->value();
        double double_v;
        int int_v;
        if (Strings::convert(value_i, double_v) != 0) // double value
          ws_iter->second->mutableRun().addProperty(name_i, double_v, true);
        else if (Strings::convert(value_i, int_v) != 0)
          ws_iter->second->mutableRun().addProperty(name_i, int_v, true);
        else
          ws_iter->second->mutableRun().addProperty(name_i, value_i, true);
      }
    }
  } // end for
  return;
}
*/

// DEBUG: mark for deletion
// /* Split ALL the TimeSeriesProperty sample logs to all the output workspace
//  * @brief FilterEvents::splitTimeSeriesLogs
//  * @param int_tsp_vector :: vector of integer tsp
//  * @param dbl_tsp_vector :: vector of double tsp
//  * @param bool_tsp_vector :: vector of boolean tsp
//  * @param string_tsp_vector :: vector of string tsp
//  */
/*
void FilterEvents::splitTimeSeriesLogs(const std::vector<TimeSeriesProperty<int> *> &int_tsp_vector,
                                       const std::vector<TimeSeriesProperty<double> *> &dbl_tsp_vector,
                                       const std::vector<TimeSeriesProperty<bool> *> &bool_tsp_vector,
                                       const std::vector<TimeSeriesProperty<string> *> &string_tsp_vector) {
  // get split times by converting vector of int64 to Time
  std::vector<Types::Core::DateAndTime> split_datetime_vec;

  // convert splitters workspace to vectors used by TableWorkspace and
  // MatrixWorkspace splitters
  if (m_useSplittersWorkspace) {
    convertSplittersWorkspaceToVectors();
  }

  // convert splitter time vector to DateAndTime format
  split_datetime_vec.resize(m_vecSplitterTime.size());
  for (size_t i = 0; i < m_vecSplitterTime.size(); ++i) {
    DateAndTime split_time(m_vecSplitterTime[i]);
    split_datetime_vec[i] = split_time;
  }

  // find the maximum index of the outputs' index
  std::set<int>::iterator target_iter;
  int max_target_index = 0;
  for (target_iter = m_targetWorkspaceIndexSet.begin(); target_iter != m_targetWorkspaceIndexSet.end(); ++target_iter) {
    if (*target_iter > max_target_index)
      max_target_index = *target_iter;
  }
  g_log.information() << "Maximum target index = " << max_target_index << "\n";

  // splitters workspace need to have 1 more for left-over events
  if (m_useSplittersWorkspace)
    ++max_target_index;

  // deal with integer time series property
  for (const auto &int_tsp : int_tsp_vector) {
    splitTimeSeriesProperty(int_tsp, split_datetime_vec, max_target_index);
  }

  // split double time series property
  for (const auto &dbl_tsp : dbl_tsp_vector) {
    splitTimeSeriesProperty(dbl_tsp, split_datetime_vec, max_target_index);
  }

  // deal with bool time series property
  for (const auto &bool_tsp : bool_tsp_vector) {
    splitTimeSeriesProperty(bool_tsp, split_datetime_vec, max_target_index);
  }

  // deal with string time series property
  for (const auto &string_tsp : string_tsp_vector) {
    splitTimeSeriesProperty(string_tsp, split_datetime_vec, max_target_index);
  }

  // integrate proton charge
  for (int tindex = 0; tindex <= max_target_index; ++tindex) {
    // find output workspace
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;
    wsiter = m_outputWorkspacesMap.find(tindex);
    if (wsiter == m_outputWorkspacesMap.end()) {
      g_log.information() << "Workspace target (indexed as " << tindex << ") does not have workspace associated.\n";
    } else {
      DataObjects::EventWorkspace_sptr ws_i = wsiter->second;
      if (ws_i->run().hasProperty("proton_charge")) {
        ws_i->mutableRun().integrateProtonCharge();
      }
    }
  }

  return;
}
*/

// DEBUG: mark for deletion
// /* split one single time-series property (template)
//  * @brief FilterEvents::splitTimeSeriesProperty
//  * @param tsp :: a time series property instance
//  * @param split_datetime_vec :: splitter
//  * @param max_target_index :: maximum number of separated time series
//  */
/*
template <typename TYPE>
void FilterEvents::splitTimeSeriesProperty(Kernel::TimeSeriesProperty<TYPE> *tsp,
                                           std::vector<Types::Core::DateAndTime> &split_datetime_vec,
                                           const int max_target_index) {
  // skip the sample logs if they are specified
  // get property name and etc
  const std::string &property_name = tsp->name();
  // generate new propertys for the source to split to
  std::vector<std::unique_ptr<TimeSeriesProperty<TYPE>>> output_vector;
  for (int tindex = 0; tindex <= max_target_index; ++tindex) {
    auto new_property = std::make_unique<TimeSeriesProperty<TYPE>>(property_name);
    new_property->setUnits(tsp->units());
    output_vector.emplace_back(std::move(new_property));
  }

  // duplicate the time series property if the size is just one
  if (tsp->size() == 1) {
    // duplicate
    for (size_t i_out = 0; i_out < output_vector.size(); ++i_out) {
      output_vector[i_out]->addValue(tsp->firstTime(), tsp->firstValue());
    }
  } else {
    // split log
    std::vector<TimeSeriesProperty<TYPE> *> split_properties(output_vector.size());
    // use vector of raw pointers for splitting
    std::transform(output_vector.begin(), output_vector.end(), split_properties.begin(),
                   [](const std::unique_ptr<TimeSeriesProperty<TYPE>> &x) { return x.get(); });
    tsp->splitByTimeVector(split_datetime_vec, m_vecSplitterGroup, split_properties);
  }

  // assign to output workspaces
  for (int tindex = 0; tindex <= max_target_index; ++tindex) {
    // find output workspace
    auto wsiter = m_outputWorkspacesMap.find(tindex);
    if (wsiter == m_outputWorkspacesMap.end()) {
      // unable to find workspace associated with target index
      g_log.information() << "Workspace target (" << tindex << ") does not have workspace associated."
                          << "\n";
    } else {
      // add property to the associated workspace
      DataObjects::EventWorkspace_sptr ws_i = wsiter->second;
      ws_i->mutableRun().addProperty(std::move(output_vector[tindex]), true);
    }
  }

  return;
}
*/

//-------------------------------------------------------------------
/** Convert splitters specified by an input workspace to TimeSplitter
 *  Cache a set of all target workspace indexes
 * @brief FilterEvents::parseInputSplitters
 */
void FilterEvents::parseInputSplitters() {
  if (m_useSplittersWorkspace)
    m_timeSplitter = TimeSplitter(m_splittersWorkspace);
  else if (m_useArbTableSplitters)
    m_timeSplitter =
        TimeSplitter(m_splitterTableWorkspace, m_isSplittersRelativeTime ? m_runStartTime : DateAndTime(0));
  else
    m_timeSplitter = TimeSplitter(m_matrixSplitterWS, m_isSplittersRelativeTime ? m_runStartTime : DateAndTime(0));

  m_targetWorkspaceIndexSet = m_timeSplitter.outputWorkspaceIndices();

  // For completeness, make sure we have a special workspace index for unfiltered events
  m_targetWorkspaceIndexSet.insert(TimeSplitter::NO_TARGET);
}

// //----------------------------------------------------------------------------------------------
// /** Convert SplittersWorkspace to vector of time and vector of target (itarget)
//  * NOTE: This is designed to use a single vector/vector splitters for all types
//  * of inputs
//  *       It is not used before vast experiment on speed comparison!
//  * @brief FilterEvents::convertSplittersWorkspaceToVectors
//  */
// void FilterEvents::convertSplittersWorkspaceToVectors() {
//   // check: only applied for splitters given by SplittersWorkspace
//   assert(m_useSplittersWorkspace);

//   // clear and get ready
//   m_vecSplitterGroup.clear();
//   m_vecSplitterTime.clear();

//   // define filter-left target index
//   int no_filter_index = m_maxTargetIndex + 1;

//   // convert TimeSplitter to a vector of sorted splitting intervals with valid targets
//   Kernel::SplittingIntervalVec splittingIntervals =
//       m_timeSplitter.toSplitters(false /*do not include "no target" intervals*/);

//   size_t num_splitters = splittingIntervals.size();
//   int64_t last_entry_time(0);

//   // it is assumed that splittingIntervals is sorted by time
//   for (size_t i_splitter = 0; i_splitter < num_splitters; ++i_splitter) {
//     // get splitter
//     Kernel::SplittingInterval splitter = splittingIntervals[i_splitter];
//     int64_t start_time_i64 = splitter.start().totalNanoseconds();
//     int64_t stop_time_i64 = splitter.stop().totalNanoseconds();
//     if (m_vecSplitterTime.empty()) {
//       // first entry: add
//       m_vecSplitterTime.emplace_back(start_time_i64);
//       m_vecSplitterTime.emplace_back(stop_time_i64);
//       m_vecSplitterGroup.emplace_back(splitter.index());
//     } else if (abs(last_entry_time - start_time_i64) < TOLERANCE) {
//       // start time is SAME as last entry
//       m_vecSplitterTime.emplace_back(stop_time_i64);
//       m_vecSplitterGroup.emplace_back(splitter.index());
//     } else if (start_time_i64 > last_entry_time + TOLERANCE) {
//       // start time is way behind. then add an empty one
//       m_vecSplitterTime.emplace_back(start_time_i64);
//       m_vecSplitterTime.emplace_back(stop_time_i64);
//       m_vecSplitterGroup.emplace_back(no_filter_index);
//       m_vecSplitterGroup.emplace_back(splitter.index());
//     } else {
//       // some impossible situation
//       std::stringstream errorss;
//       errorss << "New start time " << start_time_i64 << " is before last entry's time " << last_entry_time;
//       throw std::runtime_error(errorss.str());
//     }

//     // update
//     last_entry_time = m_vecSplitterTime.back();
//   } // END-FOR (add all splitters)
// }

//-------------------------------------------------------------------------
/** Create a list of EventWorkspace objects to be used as event filtering output.
 * Currently this method is used with SplittersWorkspace and TableWorkspace input splitters.
 * Sets the TimeROI for each destination workspace
 */
void FilterEvents::createOutputWorkspaces() {
  const auto startTime = std::chrono::high_resolution_clock::now();

  // When input is SplittersWorkspace or MatrixWorkspace, index -1 (i.e. no target specified) is present
  // When input is TableWorkspace, index -1 is not present
  size_t min_expected_number_of_indexes = (m_useSplittersWorkspace || !m_useArbTableSplitters) ? 2 : 1;
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

  int delta_wsindex{0};
  const bool from1 = getProperty("OutputWorkspaceIndexedFrom1");
  const bool outputUnfiltered = getProperty("OutputUnfilteredEvents");

  if (m_useSplittersWorkspace && from1) {
    // Determine the minimum valid target workspace index. Note that the set is sorted and guaranteed to start with -1.
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
  std::shared_ptr<EventWorkspace> templateWorkspace = this->createTemplateOutputWorkspace();
  /*
  // DEBUG: block for comparison between properties of the input workspace and the template workspace
  const auto &run = templateWorkspace->run();
  for (const auto &prop : run.getProperties()){
    const std::string &name = prop->name();
    const auto &prop_original = m_eventWS->run().getProperty(name);
    std::cout << "name = " << name << std::endl;
    if (name == "duration"){
      prop->setValue("42.0");
    }
    if (name == "slow_int_log"){
      auto *log = dynamic_cast<TimeSeriesProperty<int> *>(prop);
      log->addValue("1990-Jan-01 00:00:20.500", 42);
      std::cout << "name = " << name << std::endl;
    }
  }
  */

  // Set up target workspaces
  size_t number_of_output_workspaces{0};
  double progress_step_total =
      static_cast<double>(m_targetWorkspaceIndexSet.size()); // total number of progress steps expected
  double progress_step_current{0.};                          // current number of progress steps
  const auto originalROI = m_eventWS->run().getTimeROI();

  for (auto const wsindex : m_targetWorkspaceIndexSet) {

    //
    // Generate new workspace name
    bool add2output = true;
    std::stringstream wsname;
    wsname << m_outputWSNameBase << "_";
    if (wsindex > TimeSplitter::NO_TARGET) {
      if (descriptiveNames && splitByTime) {
        TimeROI timeROI = m_timeSplitter.getTimeROI(wsindex);
        auto splittingIntervals = timeROI.toSplitters();
        for (size_t ii = 0; ii < splittingIntervals.size(); ii++) {
          auto startTimeInSeconds =
              Mantid::Types::Core::DateAndTime::secondsFromDuration(splittingIntervals[ii].start() - m_runStartTime);
          auto stopTimeInSeconds =
              Mantid::Types::Core::DateAndTime::secondsFromDuration(splittingIntervals[ii].stop() - m_runStartTime);
          wsname << startTimeInSeconds << "_" << stopTimeInSeconds;
          if (ii < splittingIntervals.size() - 1)
            wsname << "_";
        }
      } else if (descriptiveNames) {
        auto infoiter = infomap.find(wsindex);
        if (infoiter != infomap.end()) {
          std::string name = infoiter->second;
          name = Kernel::Strings::removeSpace(name);
          wsname << name;
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

    //
    // Find the TimeROI associated to the current destination-workspace index.
    TimeROI roi{m_timeSplitter.getTimeROI(wsindex)};
    roi.update_or_replace_intersection(originalROI);
    optws->mutableRun().setTimeROI(roi);

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
      // Generate the name of the output property
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
        cout << "Created output property " << outputWorkspacePropertyName.str() << " for workspace with target index "
             << wsindex << std::endl;
      }

      ++number_of_output_workspaces;
      g_log.debug() << "Created output Workspace with target index = " << wsindex << std::endl;

      // Update progress report
      m_progress = 0.1 + 0.1 * progress_step_current / progress_step_total;
      progress(m_progress, "Creating output workspace");
      progress_step_current += 1.;
    }

  } // end of the loop iterating over the elements of m_targetWorkspaceIndexSet

  setProperty("NumberOutputWS", static_cast<int>(number_of_output_workspaces));

  addTimer("createOutputWorkspacesSplitters", startTime, std::chrono::high_resolution_clock::now());

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

void FilterEvents::filterEventsBySplitters(double progressamount) {
  const auto startTime = std::chrono::high_resolution_clock::now();
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  g_log.debug() << "Number of spectra in input/source EventWorkspace = " << numberOfSpectra << ".\n";

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws) {
    PARALLEL_START_INTERRUPT_REGION
    if (!m_vecSkip[iws]) {                                      // Filter the non-skipped
      std::map<int, DataObjects::EventList *> partialEvenLists; // event lists receiving the events from input list
      PARALLEL_CRITICAL(build_elist) {
        for (auto &ws : m_outputWorkspacesMap) {
          int index = ws.first;
          auto &partialEventList = ws.second->getSpectrum(iws);
          partialEvenLists.emplace(index, &partialEventList);
        }
        const DataObjects::EventList &inputEventList = m_eventWS->getSpectrum(iws); // input event list
        const bool pulseTof{!m_filterByPulseTime};                                  // split by pulse-time + TOF ?
        const bool tofCorrect{m_tofCorrType != NoneCorrect}; // apply corrections to the TOF values?
        m_timeSplitter.splitEventList(inputEventList, partialEvenLists, pulseTof, tofCorrect, m_detTofFactors[iws],
                                      m_detTofOffsets[iws]);
      }
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
  progress(0.1 + progressamount, "Splitting logs");
  addTimer("filterEventsBySplitters", startTime, std::chrono::high_resolution_clock::now());
}

/**
 * Clone the input workspace but with no events. Also and if necessary, only with selected logs
 *
 * @return a workspace to be cloned for each of the destination workspaces
 */
std::shared_ptr<DataObjects::EventWorkspace> FilterEvents::createTemplateOutputWorkspace() const {
  // the workspace to be returned
  std::shared_ptr<EventWorkspace> templateWorkspace = create<EventWorkspace>(*m_eventWS);

  // In not empty, "TimeSeriesPropertyLogs" specifies which logs will either be excluded or kept
  std::vector<std::string> relevantLogsList = getProperty("TimeSeriesPropertyLogs");
  // cast the list into a set
  std::set<std::string> relevantLogsSet(relevantLogsList.begin(), relevantLogsList.end());
  const size_t relevantLogsCount{relevantLogsSet.size()};

  // just clone the input outputRun if we want all the logs
  if (relevantLogsCount == 0) {
    templateWorkspace->mutableRun() = m_eventWS->run();
    return templateWorkspace;
  }

  // Exclude or only copy the relevant logs
  templateWorkspace->setSharedRun(Kernel::make_cow<Run>()); // clean-slate for the outputRun object
  auto &outputRun = templateWorkspace->mutableRun();
  const auto &inputRun = m_eventWS->run();
  outputRun.setTimeROI(inputRun.getTimeROI()); // copy the input TimeROI
  // "ExcludeSpecifiedLogs" determines if the relevant logs are to be excluded or are to be kept
  bool excludeRelevantLogs = getProperty("ExcludeSpecifiedLogs");
  bool overwriteExistingProperty{true};
  for (auto *inputProperty : inputRun.getProperties()) {
    const std::string name = inputProperty->name();
    bool logIsRelevant{relevantLogsSet.find(name) != relevantLogsSet.end()};
    bool keepOnlyRelevantLogs{!excludeRelevantLogs};
    if ((excludeRelevantLogs && logIsRelevant) || (keepOnlyRelevantLogs && !logIsRelevant))
      g_log.warning() << "Sample log " << name << "will be absent in the filtered workspace(s)\n";
    else
      outputRun.addProperty(inputProperty, overwriteExistingProperty);
  }
  return templateWorkspace;
}

// //----------------------------------------------------------------------------------------------
// /* Generate a vector of integer time series property for each splitter
//  * corresponding to each target (in integer)
//  * in each splitter-time-series-property, 1 stands for include and 0 stands
//  * for time for neutrons to be discarded. If there is no UN-DEFINED
//  * @brief FilterEvents::generateSplitterTSP
//  * @param split_tsp_vec
//  */
// void FilterEvents::generateSplitterTSP(std::vector<std::unique_ptr<Kernel::TimeSeriesProperty<int>>> &split_tsp_vec)
// {
//   // clear vector to set up
//   split_tsp_vec.clear();

//   // initialize m_maxTargetIndex + 1 time series properties in integer

//   for (int itarget = 0; itarget <= m_maxTargetIndex; ++itarget) {
//     auto split_tsp = std::make_unique<Kernel::TimeSeriesProperty<int>>("splitter");
//     // add initial value if the first splitter time is after the run start
//     // time
//     split_tsp->addValue(Types::Core::DateAndTime(m_runStartTime), 0);
//     split_tsp_vec.emplace_back(std::move(split_tsp));
//   }

//   // start to go through  m_vecSplitterTime (int64) and m_vecSplitterGroup add
//   // each entry to corresponding splitter TSP
//   for (size_t igrp = 0; igrp < m_vecSplitterGroup.size(); ++igrp) {
//     // get the target workspace's index and the starting
//     int itarget = m_vecSplitterGroup[igrp];
//     // start time of the entry with value 1
//     DateAndTime start_time(m_vecSplitterTime[igrp]);
//     // identify by case
//     if (start_time < m_runStartTime) {
//       // too early, ignore
//       continue;
//     }

//     // get the current TSP
//     Kernel::TimeSeriesProperty<int> *curr_tsp = split_tsp_vec[itarget].get();

//     if (start_time == m_runStartTime) {
//       // just same as the run start time: there must be one and only 1 entry
//       // with value 0
//       if (curr_tsp->size() != 1) {
//         std::stringstream error;
//         error << "Splitter TSP for target workspace " << itarget << " must have 1 and only 1 entry "
//               << "if there is a splitter right at its run start time.";
//         throw std::runtime_error(error.str());
//       }
//       // the first entry should have an entry with value 0
//       if (curr_tsp->firstValue() != 0) {
//         std::stringstream error;
//         error << "Splitter TSP for target workspace " << itarget << " must have 1 and only 1 entry "
//               << "with value as 0 but not " << curr_tsp->firstValue() << " if there is a splitter "
//               << "right at its run start time.";
//         throw std::runtime_error(error.str());
//       }
//       // replace the value with '1' by clearing the original one first
//       curr_tsp->clear();
//     }

//     // later than the run start time, then add a new entry
//     curr_tsp->addValue(start_time, 1);

//     // add run stop time as a new entry
//     DateAndTime stop_time(m_vecSplitterTime[igrp + 1]);
//     curr_tsp->addValue(stop_time, 0);
//   }

//   return;
// }

// DEBUG: mark this member function for deletion
// /* Generate the splitter's time series property (log) the splitters workspace
//  * @brief FilterEvents::generateSplitterTSPalpha
//  * @param split_tsp_vec
//  */
/*
void FilterEvents::generateSplitterTSPalpha(
    std::vector<std::unique_ptr<Kernel::TimeSeriesProperty<int>>> &split_tsp_vec) {
  // clear vector to set up
  split_tsp_vec.clear();

  // initialize m_maxTargetIndex + 1 time series properties in integer
  // TODO:FIXME - shall not use m_maxTargetIndex, because it is not set for
  // SplittersWorkspace-type splitters
  g_log.debug() << "Maximum target index = " << m_maxTargetIndex << "\n";
  if (m_maxTargetIndex < 0)
    throw std::runtime_error("Maximum target index cannot be negative.");

  // initialize the target index
  for (int itarget = 0; itarget <= m_maxTargetIndex; ++itarget) {
    auto split_tsp = std::make_unique<Kernel::TimeSeriesProperty<int>>("splitter");
    split_tsp->addValue(m_runStartTime, 0);
    split_tsp_vec.emplace_back(std::move(split_tsp));
  }

  for (auto const itarget : m_targetWorkspaceIndexSet) {
    if (itarget == TimeSplitter::NO_TARGET)
      continue;

    TimeROI timeROI = m_timeSplitter.getTimeROI(itarget);

    Kernel::SplittingIntervalVec splittingIntervalVec = timeROI.toSplitters();
    for (auto const splitter : splittingIntervalVec) {
      if (itarget >= static_cast<int>(split_tsp_vec.size()))
        throw std::runtime_error("Target workspace index is out of range!");

      if (splitter.start() == m_runStartTime) {
        // there should be only 1 value in the splitter and clear it.
        if (split_tsp_vec[itarget]->size() != 1) {
          throw std::runtime_error("Splitter must have 1 value with initialization.");
        }
        split_tsp_vec[itarget]->clear();
      }
      split_tsp_vec[itarget]->addValue(splitter.start(), 1);
      split_tsp_vec[itarget]->addValue(splitter.stop(), 0);
    }
  }
}
*/

// DEBUG: mark for deletion FilterEvents::mapSplitterTSPtoWorkspaces
// /* add the splitter TimeSeriesProperty logs to each workspace
//  * @brief FilterEvents::mapSplitterTSPtoWorkspaces
//  * @param split_tsp_vec
//  */
/*
void FilterEvents::mapSplitterTSPtoWorkspaces(
    std::vector<std::unique_ptr<Kernel::TimeSeriesProperty<int>>> &split_tsp_vec) {
  g_log.debug() << "There are " << split_tsp_vec.size() << " TimeSeriesPropeties.\n"
                << "There are " << m_outputWorkspacesMap.size() << " Output worskpaces.\n";

  if (split_tsp_vec.size() != m_outputWorkspacesMap.size() - 1) {
    g_log.warning() << "Number of Splitter vector (" << split_tsp_vec.size()
                    << ") does not match number of filtered output workspaces (" << m_outputWorkspacesMap.size() - 1
                    << ")\n";
  }

  for (int itarget = 0; itarget < static_cast<int>(split_tsp_vec.size()); ++itarget) {
    // use itarget to find the workspace that is mapped
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator ws_iter;
    ws_iter = m_outputWorkspacesMap.find(itarget);

    // skip if an itarget does not have matched workspace
    if (ws_iter == m_outputWorkspacesMap.end()) {
      g_log.warning() << "iTarget " << itarget << " does not have any workspace associated.\n";
      continue;
    }

    // get the workspace
    DataObjects::EventWorkspace_sptr outws = ws_iter->second;

    // calculate the duration
    double duration = calculate_duration(split_tsp_vec[itarget]);

    // add property
    PropertyWithValue<double> *duration_property = new PropertyWithValue<double>("duration", duration);
    outws->mutableRun().addProperty(duration_property, true);
    // note: split_tps_vec[i], the shared pointer, will be destroyed by
    // std::move()
    outws->mutableRun().addProperty(std::move(split_tsp_vec[itarget]), true);
  }

  return;
}
*/

// DEBUG: mark for deletion FilterEvents::calculate_duration
// /* Calculate split-workspace's duration according to splitter time series
//  * property
//  * @brief calculate the duration from TSP "splitter"
//  * @param splitter_tsp :: TimeSeriesProperty for splitter
//  * @return
//  */
/*
double FilterEvents::calculate_duration(std::unique_ptr<Kernel::TimeSeriesProperty<int>> &splitter_tsp) {
  // Get the times and values
  std::vector<int> split_values = splitter_tsp->valuesAsVector();
  std::vector<DateAndTime> split_time = splitter_tsp->timesAsVector();

  double duration = 0.;
  for (size_t i = 0; i < split_values.size() - 1; ++i) {
    // for splitter's value == 1 (from this till 0 will be counted in the
    // duration)
    if (split_values[i] == 1) {
      // difference in nanosecond and then converted to second
      double sub_duration =
          1.E-9 * static_cast<double>(split_time[i + 1].totalNanoseconds() - split_time[i].totalNanoseconds());
      // increment
      duration += sub_duration;
    }
  }

  return duration;
}
*/

/** Get all filterable logs' names (double and integer)
 * @returns Vector of names of logs
 */
std::vector<std::string> FilterEvents::getTimeSeriesLogNames() {
  std::vector<std::string> lognames;

  const std::vector<Kernel::Property *> allprop = m_eventWS->mutableRun().getProperties();
  for (auto ip : allprop) {
    // cast to double log and integer log
    auto *dbltimeprop = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(ip);
    auto *inttimeprop = dynamic_cast<Kernel::TimeSeriesProperty<int> *>(ip);
    auto *booltimeprop = dynamic_cast<Kernel::TimeSeriesProperty<bool> *>(ip);

    // append to vector if it is either double TimeSeries or int TimeSeries
    if (dbltimeprop || inttimeprop || booltimeprop) {
      const std::string &pname = ip->name();
      lognames.emplace_back(pname);
    }
  }

  return lognames;
}

} // namespace Mantid::Algorithms
