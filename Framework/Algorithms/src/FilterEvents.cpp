#include "MantidAlgorithms/FilterEvents.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"
#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include <memory>
#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

const int64_t TOLERANCE(1000000); // splitter time tolerance in nano-second.
                                  // this value has resolution to 10000Hz

/// (integer) splitting target for undefined region, which will be recorded in
/// m_splitterGroup
const uint32_t UNDEFINED_SPLITTING_TARGET(0);

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(FilterEvents)

/** Constructor
 */
FilterEvents::FilterEvents()
    : m_eventWS(), m_splittersWorkspace(), m_splitterTableWorkspace(),
      m_matrixSplitterWS(), m_detCorrectWorkspace(),
      m_useSplittersWorkspace(false), m_useArbTableSplitters(false),
      m_targetWorkspaceIndexSet(), m_splitters(), m_outputWorkspacesMap(),
      m_wsNames(), m_detTofOffsets(), m_detTofFactors(),
      m_filterByPulseTime(false), m_informationWS(), m_hasInfoWS(),
      m_progress(0.), m_outputWSNameBase(), m_toGroupWS(false),
      m_vecSplitterTime(), m_vecSplitterGroup(), m_splitSampleLogs(false),
      m_useDBSpectrum(false), m_dbWSIndex(-1), m_tofCorrType(),
      m_specSkipType(), m_vecSkip(), m_isSplittersRelativeTime(false),
      m_filterStartTime(0), m_runStartTime(0) {}

/** Declare Inputs
 */
void FilterEvents::init() {
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(Kernel::make_unique<API::WorkspaceProperty<API::Workspace>>(
                      "SplitterWorkspace", "", Direction::Input),
                  "An input SpilltersWorskpace for filtering");

  declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                  "The base name to use for the output workspace");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<TableWorkspace>>(
          "InformationWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional output for the information of each splitter "
      "workspace index.");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputTOFCorrectionWorkspace", "TOFCorrectWS", Direction::Output),
      "Name of output workspace for TOF correction factor. ");

  declareProperty("FilterByPulseTime", false,
                  "Filter the event by its pulse time only for slow sample "
                  "environment log.  This option can make execution of "
                  "algorithm faster.  But it lowers precision.");

  declareProperty("GroupWorkspaces", false, "Option to group all the output "
                                            "workspaces.  Group name will be "
                                            "OutputWorkspaceBaseName.");

  declareProperty("OutputWorkspaceIndexedFrom1", false,
                  "If selected, the minimum output workspace is indexed from 1 "
                  "and continuous. ");

  // TOF correction
  vector<string> corrtypes{"None", "Customized", "Direct", "Elastic",
                           "Indirect"};
  declareProperty("CorrectionToSample", "None",
                  boost::make_shared<StringListValidator>(corrtypes),
                  "Type of correction on neutron events to sample time from "
                  "detector time. ");

  declareProperty(Kernel::make_unique<WorkspaceProperty<TableWorkspace>>(
                      "DetectorTOFCorrectionWorkspace", "", Direction::Input,
                      PropertyMode::Optional),
                  "Name of table workspace containing the log "
                  "time correction factor for each detector. ");
  setPropertySettings("DetectorTOFCorrectionWorkspace",
                      Kernel::make_unique<VisibleWhenProperty>(
                          "CorrectionToSample", IS_EQUAL_TO, "Customized"));

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
                  "Value of incident energy (Ei) in meV in direct mode.");
  setPropertySettings("IncidentEnergy",
                      Kernel::make_unique<VisibleWhenProperty>(
                          "CorrectionToSample", IS_EQUAL_TO, "Direct"));

  // Algorithm to spectra without detectors
  vector<string> spec_no_det{"Skip", "Skip only if TOF correction"};
  declareProperty("SpectrumWithoutDetector", "Skip",
                  boost::make_shared<StringListValidator>(spec_no_det),
                  "Approach to deal with spectrum without detectors. ");

  declareProperty("SplitSampleLogs", true,
                  "If selected, all sample logs will be splitted by the  "
                  "event splitters.  It is not recommended for fast event "
                  "log splitters. ");

  declareProperty("NumberOutputWS", 0,
                  "Number of output output workspace splitted. ",
                  Direction::Output);

  declareProperty("DBSpectrum", EMPTY_INT(),
                  "Spectrum (workspace index) for debug purpose. ");

  declareProperty(Kernel::make_unique<ArrayProperty<string>>(
                      "OutputWorkspaceNames", Direction::Output),
                  "List of output workspaces names");

  declareProperty(
      "RelativeTime", false,
      "Flag to indicate that in the input Matrix splitting workspace,"
      "the time indicated by X-vector is relative to either run start time or "
      "some indicted time.");

  declareProperty(
      "FilterStartTime", "",
      "Start time for splitters that can be parsed to DateAndTime.");
}

/** Execution body
 */
void FilterEvents::exec() {
  // Process algorithm properties
  processAlgorithmProperties();

  // Examine workspace for detectors
  examineAndSortEventWS();

  // Parse splitters
  m_progress = 0.0;
  progress(m_progress, "Processing SplittersWorkspace.");
  if (m_useSplittersWorkspace)
    processSplittersWorkspace();
  else if (m_useArbTableSplitters)
    processTableSplittersWorkspace();
  else
    processMatrixSplitterWorkspace();

  // Create output workspaces
  m_progress = 0.1;
  progress(m_progress, "Create Output Workspaces.");
  if (m_useArbTableSplitters)
    createOutputWorkspacesTableSplitterCase();
  else if (m_useSplittersWorkspace)
    createOutputWorkspaces();
  else
    createOutputWorkspacesMatrixCase();

  // Optionall import corrections
  m_progress = 0.20;
  progress(m_progress, "Importing TOF corrections. ");
  setupDetectorTOFCalibration();

  // Filter Events
  m_progress = 0.30;
  progress(m_progress, "Filter Events.");
  double progressamount;
  if (m_toGroupWS)
    progressamount = 0.6;
  else
    progressamount = 0.7;

  std::vector<Kernel::TimeSeriesProperty<int> *> split_tsp_vector;
  if (m_useSplittersWorkspace) {
    filterEventsBySplitters(progressamount);
    generateSplitterTSPalpha(split_tsp_vector);
  } else {
    filterEventsByVectorSplitters(progressamount);
    generateSplitterTSP(split_tsp_vector);
  }

  // TODO:FIXME - assign split_tsp_vector to all the output workspaces!
  mapSplitterTSPtoWorkspaces(split_tsp_vector);

  // Optional to group detector
  // TODO:FIXME - move this part to a method
  if (m_toGroupWS) {
    m_progress = 0.9;
    progress(m_progress, "Group workspaces");

    std::string groupname = m_outputWSNameBase;
    API::IAlgorithm_sptr groupws =
        createChildAlgorithm("GroupWorkspaces", 0.99, 1.00, true);
    // groupws->initialize();
    groupws->setAlwaysStoreInADS(true);
    groupws->setProperty("InputWorkspaces", m_wsNames);
    groupws->setProperty("OutputWorkspace", groupname);
    groupws->execute();
    if (!groupws->isExecuted()) {
      g_log.error() << "Grouping all output workspaces fails.\n";
    }
  }

  // TODO:FIXME - move this part to a method
  // Form the names of output workspaces
  std::vector<std::string> outputwsnames;
  std::map<int, DataObjects::EventWorkspace_sptr>::iterator miter;
  // for (miter = m_outputWorkspacesMap.begin();
  //     miter != m_outputWorkspacesMap.end(); ++miter) {
  //  outputwsnames.push_back(miter->second->name());
  for (miter = m_outputWorkspacesMap.begin();
       miter != m_outputWorkspacesMap.end(); ++miter) {
    outputwsnames.push_back(miter->second->getName());
  }
  setProperty("OutputWorkspaceNames", outputwsnames);

  m_progress = 1.0;
  progress(m_progress, "Completed");
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

  // Process splitting workspace (table or data)
  API::Workspace_sptr tempws = this->getProperty("SplitterWorkspace");

  m_splittersWorkspace =
      boost::dynamic_pointer_cast<SplittersWorkspace>(tempws);
  m_splitterTableWorkspace =
      boost::dynamic_pointer_cast<TableWorkspace>(tempws);
  if (m_splittersWorkspace) {
    m_useSplittersWorkspace = true;
  } else if (m_splitterTableWorkspace)
    m_useArbTableSplitters = true;
  else {
    m_matrixSplitterWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tempws);
    if (m_matrixSplitterWS) {
      m_useSplittersWorkspace = false;
    } else {
      throw runtime_error("Invalid type of input workspace, neither "
                          "SplittersWorkspace nor MatrixWorkspace.");
    }
  }

  m_informationWS = this->getProperty("InformationWorkspace");
  // Informatin workspace is specified?
  if (!m_informationWS)
    m_hasInfoWS = false;
  else
    m_hasInfoWS = true;

  m_outputWSNameBase = this->getPropertyValue("OutputWorkspaceBaseName");
  m_filterByPulseTime = this->getProperty("FilterByPulseTime");

  m_toGroupWS = this->getProperty("GroupWorkspaces");

  //-------------------------------------------------------------------------
  // TOF detector/sample correction
  //-------------------------------------------------------------------------
  // Type of correction
  string correctiontype = getPropertyValue("CorrectionToSample");
  if (correctiontype.compare("None") == 0)
    m_tofCorrType = NoneCorrect;
  else if (correctiontype.compare("Customized") == 0)
    m_tofCorrType = CustomizedCorrect;
  else if (correctiontype.compare("Direct") == 0)
    m_tofCorrType = DirectCorrect;
  else if (correctiontype.compare("Elastic") == 0)
    m_tofCorrType = ElasticCorrect;
  else if (correctiontype.compare("Indirect") == 0)
    m_tofCorrType = IndirectCorrect;
  else {
    g_log.error() << "Correction type '" << correctiontype
                  << "' is not supported. \n";
    throw runtime_error("Impossible situation!");
  }

  if (m_tofCorrType == CustomizedCorrect) {
    // Customized correciton
    m_detCorrectWorkspace = getProperty("DetectorTOFCorrectionWorkspace");
    if (!m_detCorrectWorkspace)
      throw runtime_error("In case of customized TOF correction, correction "
                          "workspace must be given!");
  }

  // Spectrum skip
  string skipappr = getPropertyValue("SpectrumWithoutDetector");
  if (skipappr.compare("Skip") == 0)
    m_specSkipType = EventFilterSkipNoDet;
  else if (skipappr.compare("Skip only if TOF correction") == 0)
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

  // Get run start time
  if (m_eventWS->run().hasProperty("run_start")) {
    Kernel::DateAndTime run_start_time(
        m_eventWS->run().getProperty("run_start")->value());
    m_runStartTime = run_start_time;
  }

  // Splitters are given relative time
  m_isSplittersRelativeTime = getProperty("RelativeTime");
  if (m_isSplittersRelativeTime) {
    // Using relative time
    std::string start_time_str = getProperty("FilterStartTime");
    if (!start_time_str.empty()) {
      // User specifies the filter starting time
      Kernel::DateAndTime temp_shift_time(start_time_str);
      m_filterStartTime = temp_shift_time;
    } else {
      // Retrieve filter starting time from property run_start as default
      if (m_eventWS->run().hasProperty("run_start")) {
        m_filterStartTime = m_runStartTime;
      } else {
        throw std::runtime_error(
            "Input event workspace does not have property run_start. "
            "User does not specifiy filter start time."
            "Splitters cannot be in reltive time.");
      }
    }
  } // END-IF: m_isSplitterRelativeTime
}

//----------------------------------------------------------------------------------------------
/**  Examine whether any spectrum does not have detector
 * Warning message will be written out
 * @brief FilterEvents::examineEventWS
 */
void FilterEvents::examineAndSortEventWS() {
  // get event workspace information
  size_t numhist = m_eventWS->getNumberHistograms();
  m_vecSkip.resize(numhist, false);

  // check whether any detector is skipped
  if (m_specSkipType == EventFilterSkipNoDetTOFCorr &&
      m_tofCorrType == NoneCorrect) {
    // No TOF correction and skip spectrum only if TOF correction is required
    g_log.warning(
        "By user's choice, No spectrum will be skipped even if it has "
        "no detector.");
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
      g_log.warning()
          << "There are " << numskipspec
          << " spectra that do not have detectors. "
          << "They will be skipped during filtering. There are total "
          << numeventsskip
          << " events in those spectra. \nList of these specta is as below:\n"
          << msgss.str() << "\n";
    } else {
      g_log.notice("There is no spectrum that does not have detectors.");
    }

  } // END-IF-ELSE

  // sort events
  DataObjects::EventSortType sortType = DataObjects::TOF_SORT;
  if (m_filterByPulseTime)
    sortType = DataObjects::PULSETIME_SORT;
  else
    sortType = DataObjects::PULSETIMETOF_SORT;

  // This runs the SortEvents algorithm in parallel
  m_eventWS->sortAll(sortType, nullptr);

  return;
}

//----------------------------------------------------------------------------------------------
/** Purpose:
 *    Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
 *    and create a map for all workspace group number
 *  Requirements:
 *  Gaurantees:
 *  - Update of m_maxTargetIndex: it can be zero in SplittersWorkspace case
 * @brief FilterEvents::processSplittersWorkspace
 */
void FilterEvents::processSplittersWorkspace() {
  // 1. Init data structure
  size_t numsplitters = m_splittersWorkspace->getNumberSplitters();
  m_splitters.reserve(numsplitters);

  // 2. Insert all splitters
  m_maxTargetIndex = 0;
  bool inorder = true;
  for (size_t i = 0; i < numsplitters; i++) {
    // push back the splitter in SplittersWorkspace to list of splitters
    m_splitters.push_back(m_splittersWorkspace->getSplitter(i));
    // add the target workspace index to target workspace indexes set
    m_targetWorkspaceIndexSet.insert(m_splitters.back().index());
    // register for the maximum target index
    if (m_splitters.back().index() > m_maxTargetIndex)
      m_maxTargetIndex = m_splitters.back().index();
    // check whether the splitters are in time order
    if (inorder && i > 0 && m_splitters[i] < m_splitters[i - 1])
      inorder = false;
  }
  m_progress = 0.05;
  progress(m_progress);

  // 3. Order if not ordered and add workspace for events excluded
  if (!inorder) {
    std::sort(m_splitters.begin(), m_splitters.end());
  }

  // 4. Add extra workgroup index for unfiltered events
  m_targetWorkspaceIndexSet.insert(-1);

  // 5. Add information
  if (m_hasInfoWS) {
    if (m_targetWorkspaceIndexSet.size() > m_informationWS->rowCount() + 1) {
      g_log.warning() << "Input Splitters Workspace has different entries ("
                      << m_targetWorkspaceIndexSet.size() - 1
                      << ") than input information workspaces ("
                      << m_informationWS->rowCount() << "). "
                      << "  Information may not be accurate. \n";
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Convert SplittersWorkspace to vector of time and vector of target (itarget)
 * NOTE: This is designed to use a single vector/vector splitters for all types
 * of inputs
 *       It is not used before vast experiment on speed comparison!
 * @brief FilterEvents::convertSplittersWorkspaceToVectors
 */
void FilterEvents::convertSplittersWorkspaceToVectors() {
  // check: only applied for splitters given by SplittersWorkspace
  assert(m_useSplittersWorkspace);

  // clear and get ready
  m_vecSplitterGroup.clear();
  m_vecSplitterTime.clear();

  // convert SplittersWorkspace to a set of pairs which can be sorted
  size_t num_rows = this->m_splittersWorkspace->rowCount();
  for (size_t irow = 0; irow < num_rows; ++irow) {
    Kernel::SplittingInterval splitter =
        m_splittersWorkspace->getSplitter(irow);
    if (m_vecSplitterTime.size() == 0 ||
        splitter.start() > m_vecSplitterTime.back() + TOLERANCE) {
      m_vecSplitterTime.push_back(splitter.start().totalNanoseconds());
      m_vecSplitterTime.push_back(splitter.stop().totalNanoseconds());
      // 0 stands for not defined
      m_vecSplitterGroup.push_back(0);
      m_vecSplitterGroup.push_back(splitter.index());
    } else if (splitter.start() < m_vecSplitterTime.back() - TOLERANCE) {
      // almost same: then add the spliters.stop() only
      m_vecSplitterTime.push_back(splitter.stop().totalNanoseconds());
      m_vecSplitterGroup.push_back(splitter.index());
    } else {
      // have to insert the somewhere
      std::vector<int64_t>::iterator finditer =
          std::lower_bound(m_vecSplitterTime.begin(), m_vecSplitterTime.end(),
                           splitter.start().totalNanoseconds());
      // get the index
      size_t split_index =
          static_cast<size_t>(finditer - m_vecSplitterTime.begin());
      if (*finditer - splitter.start().totalNanoseconds() > TOLERANCE) {
        // the start time is before one splitter indicated by *finditer: insert
        // both
        // check
        if (m_vecSplitterGroup[split_index] != UNDEFINED_SPLITTING_TARGET) {
          std::stringstream errss;
          errss << "Tried to insert splitter [" << splitter.start() << ", "
                << splitter.stop() << "] but there is "
                << "already a time entry with target "
                << m_vecSplitterGroup[split_index] << " inside it.";
          throw std::runtime_error(errss.str());
        }
        // inset the full set
        m_vecSplitterTime.insert(finditer, splitter.stop().totalNanoseconds());
        m_vecSplitterTime.insert(finditer, splitter.start().totalNanoseconds());
        // insert the target
        m_vecSplitterGroup.insert(m_vecSplitterGroup.begin() + split_index,
                                  static_cast<int>(UNDEFINED_SPLITTING_TARGET));
        m_vecSplitterGroup.insert(m_vecSplitterGroup.begin() + split_index,
                                  static_cast<int>(splitter.index()));
      } else if (*finditer - splitter.start().totalNanoseconds() > -TOLERANCE) {
        // the start time is an existing entry
        // check
        if (m_vecSplitterGroup[split_index] != UNDEFINED_SPLITTING_TARGET) {
          std::stringstream errss;
          errss << "Tried to insert splitter [" << splitter.start() << ", "
                << splitter.stop() << "] but there is "
                << "already a time entry with target "
                << m_vecSplitterGroup[split_index] << " inside it.";
          throw std::runtime_error(errss.str());
        }
        // inset the stop time
        m_vecSplitterTime.insert(finditer + 1,
                                 splitter.stop().totalNanoseconds());
        // insert the target
        m_vecSplitterGroup.insert(m_vecSplitterGroup.begin() + split_index + 1,
                                  splitter.index());
      } else {
        throw std::runtime_error("This is not a possible situation!");
      }
    } // IF-ELSE to add a new entry
  }   // END-FOR (add all splitters)

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FilterEvents::processMatrixSplitterWorkspace
 * Purpose:
 *   Convert the splitters in MatrixWorkspace to m_vecSplitterTime and
 * m_vecSplitterGroup
 * Requirements:
 *   m_matrixSplitterWS has valid value
 *   vecX's size must be one larger than and that of vecY of m_matrixSplitterWS
 * Guarantees
 *  - Splitters stored in m_matrixSpliterWS are transformed to
 *    "m_vecSplitterTime" and "m_vecSplitterGroup", whose sizes differ by 1.
 *  - Y values are mapped to integer group index stored in "m_vecSplitterGroup".
 *    The mapping is recorded in "m_yIndexMap" and "m_wsGroupdYMap"
 *    "m_maxTargetIndex" is used to register the maximum group index
 *    Negative Y is defined as "undefined"
 * Note: there is NO undefined split region here, while any NEGATIVE Y value is
 * defined as "undefined splitter"
 */
void FilterEvents::processMatrixSplitterWorkspace() {
  // Check input workspace validity
  assert(m_matrixSplitterWS);

  auto X = m_matrixSplitterWS->binEdges(0);
  auto &Y = m_matrixSplitterWS->y(0);
  size_t sizex = X.size();
  size_t sizey = Y.size();

  // Assign vectors for time comparison
  m_vecSplitterTime.assign(X.size(), 0);
  m_vecSplitterGroup.assign(Y.size(), -1);

  // Transform vector
  for (size_t i = 0; i < sizex; ++i) {
    m_vecSplitterTime[i] = static_cast<int64_t>(X[i] * 1.E9);
  }
  // shift the splitters' time if user specifis that the input times are
  // relative
  if (m_isSplittersRelativeTime) {
    int64_t time_shift_ns = m_filterStartTime.totalNanoseconds();
    for (size_t i = 0; i < sizex; ++i)
      m_vecSplitterTime[i] += time_shift_ns;
  }

  // process the group
  uint32_t max_target_index = 1;

  for (size_t i = 0; i < sizey; ++i) {

    int y_index = static_cast<int>(Y[i]);

    // try to find Y[i] in m_yIndexMap
    std::map<int, uint32_t>::iterator mapiter = m_yIndexMap.find(y_index);
    if (mapiter == m_yIndexMap.end()) {
      // new
      // default to 0 as undefined slot.
      uint32_t int_target = UNDEFINED_SPLITTING_TARGET;
      //  if well-defined, then use the current
      if (y_index >= 0) {
        int_target = max_target_index;
        ++max_target_index;
      }

      // un-defined or un-filtered
      m_vecSplitterGroup[i] = int_target;

      // add to maps and etc.
      m_yIndexMap.emplace(y_index, int_target);
      m_wsGroupdYMap.emplace(int_target, y_index);
      m_targetWorkspaceIndexSet.insert(int_target);
    } else {
      // this target Y-index has been registered previously
      uint32_t target_index = mapiter->second;
      m_vecSplitterGroup[i] = target_index;
    }
  }

  // register the max target integer
  m_maxTargetIndex = max_target_index - 1;

  return;
}

//----------------------------------------------------------------------------------------------
/** process the input splitters given by a TableWorkspace
 * The method will transfer the start/stop time to "m_vecSplitterTime"
 * and map the splitting target (in string) to "m_vecSplitterGroup".
 * The mapping will be recorded in "m_targetIndexMap" and
 *"m_wsGroupIndexTargetMap".
 * Also, "m_maxTargetIndex" is set up to record the highest target group/index,
 * i.e., max value of m_vecSplitterGroup
 *
 * @brief FilterEvents::processTableSplittersWorkspace
 */
void FilterEvents::processTableSplittersWorkspace() {
  // check input workspace's validity
  assert(m_splitterTableWorkspace);
  if (m_splitterTableWorkspace->columnCount() != 3) {
    throw std::runtime_error(
        "Splitters given in TableWorkspace must have 3 columns.");
  }

  // clear vector splitterTime and vector of splitter group
  m_vecSplitterTime.clear();
  m_vecSplitterGroup.clear();
  bool found_undefined_splitter = false;

  // get the run start time
  int64_t filter_shift_time(0);
  if (m_isSplittersRelativeTime)
    filter_shift_time = m_runStartTime.totalNanoseconds();

  int max_target_index = 1;

  // convert TableWorkspace's values to vectors
  size_t num_rows = m_splitterTableWorkspace->rowCount();
  for (size_t irow = 0; irow < num_rows; ++irow) {
    // get start and stop time in second
    double start_time = m_splitterTableWorkspace->cell_cast<double>(irow, 0);
    double stop_time = m_splitterTableWorkspace->cell_cast<double>(irow, 1);
    std::string target = m_splitterTableWorkspace->cell<std::string>(irow, 2);

    int64_t start_64 =
        filter_shift_time + static_cast<int64_t>(start_time * 1.E9);
    int64_t stop_64 =
        filter_shift_time + static_cast<int64_t>(stop_time * 1.E9);

    if (m_vecSplitterTime.size() == 0) {
      // first splitter: push the start time to vector
      m_vecSplitterTime.push_back(start_64);
    } else if (start_64 - m_vecSplitterTime.back() > TOLERANCE) {
      // the start time is way behind previous splitter's stop time
      // create a new splitter and set the time interval in the middle to target
      // -1
      m_vecSplitterTime.push_back(start_64);
      // NOTE: use index = 0 for un-defined slot
      m_vecSplitterGroup.push_back(UNDEFINED_SPLITTING_TARGET);
      found_undefined_splitter = true;
    } else if (abs(start_64 - m_vecSplitterTime.back()) < TOLERANCE) {
      // new splitter's start time is same (within tolerance) as the stop time
      // of the previous
      ;
    } else {
      // new splitter's start time is before the stop time of the last splitter.
      throw std::runtime_error("Input table workspace does not have splitters "
                               "set up in order, which is a requirement.");
    }

    // convert string-target to integer target
    bool addnew = false;
    int int_target(-1);
    if (m_targetIndexMap.size() == 0) {
      addnew = true;
    } else {
      std::map<std::string, int>::iterator mapiter =
          m_targetIndexMap.find(target);
      if (mapiter == m_targetIndexMap.end())
        addnew = true;
      else
        int_target = mapiter->second;
    }

    // add a new ordered-integer-target
    if (addnew) {
      // target is not in map
      int_target = max_target_index;
      m_targetIndexMap.insert(std::pair<std::string, int>(target, int_target));
      m_wsGroupIndexTargetMap.emplace(int_target, target);
      this->m_targetWorkspaceIndexSet.insert(int_target);
      max_target_index++;
    }

    // add start time, stop time and 'target
    m_vecSplitterTime.push_back(stop_64);
    m_vecSplitterGroup.push_back(int_target);
  } // END-FOR (irow)

  // record max target index
  m_maxTargetIndex = max_target_index - 1;

  // add un-defined splitter to map
  if (found_undefined_splitter) {
    m_targetIndexMap.emplace("undefined", 0);
    m_wsGroupIndexTargetMap.emplace(0, "undefined");
    m_targetWorkspaceIndexSet.insert(0);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Create a list of EventWorkspace for output in the case that splitters are
 * given by
 *  SplittersWorkspace
 */
void FilterEvents::createOutputWorkspaces() {

  // Convert information workspace to map
  std::map<int, std::string> infomap;
  if (m_hasInfoWS) {
    for (size_t ir = 0; ir < m_informationWS->rowCount(); ++ir) {
      API::TableRow row = m_informationWS->getRow(ir);
      infomap.emplace(row.Int(0), row.String(1));
    }
  }

  // Determine the minimum group index number
  int minwsgroup = INT_MAX;
  for (auto wsgroup : m_targetWorkspaceIndexSet) {
    if (wsgroup < minwsgroup && wsgroup >= 0)
      minwsgroup = wsgroup;
  }
  g_log.debug() << "Min WS Group = " << minwsgroup << "\n";

  bool from1 = getProperty("OutputWorkspaceIndexedFrom1");
  int delta_wsindex = 0;
  if (from1) {
    delta_wsindex = 1 - minwsgroup;
  }

  // Set up new workspaces
  int numoutputws = 0;
  double numnewws = static_cast<double>(m_targetWorkspaceIndexSet.size());
  double wsgindex = 0.;

  for (auto const wsgroup : m_targetWorkspaceIndexSet) {
    // Generate new workspace name
    bool add2output = true;
    std::stringstream wsname;
    if (wsgroup >= 0) {
      wsname << m_outputWSNameBase << "_" << (wsgroup + delta_wsindex);
    } else {
      wsname << m_outputWSNameBase << "_unfiltered";
      if (from1)
        add2output = false;
    }

    boost::shared_ptr<EventWorkspace> optws =
        create<DataObjects::EventWorkspace>(*m_eventWS);
    m_outputWorkspacesMap.emplace(wsgroup, optws);

    // Add information, including title and comment, to output workspace
    if (m_hasInfoWS) {
      std::string info;
      if (wsgroup < 0) {
        info = "Events that are filtered out. ";
      } else {
        std::map<int, std::string>::iterator infoiter;
        infoiter = infomap.find(wsgroup);
        if (infoiter != infomap.end()) {
          info = infoiter->second;
        } else {
          info = "This workspace has no informatin provided. ";
        }
      }
      optws->setComment(info);
      optws->setTitle(info);
    } // END-IF infor WS

    // Add to output properties.  There shouldn't be any workspace
    // (non-unfiltered) skipped from group index
    if (add2output) {
      // Generate output property name
      std::stringstream propertynamess;
      if (wsgroup == -1) {
        propertynamess << "OutputWorkspace_unfiltered";
      } else {
        propertynamess << "OutputWorkspace_" << wsgroup;
      }

      // Inserted this pair to map
      m_wsNames.push_back(wsname.str());

      // Set (property) to output workspace and set to ADS
      declareProperty(
          Kernel::make_unique<
              API::WorkspaceProperty<DataObjects::EventWorkspace>>(
              propertynamess.str(), wsname.str(), Direction::Output),
          "Output");
      setProperty(propertynamess.str(), optws);
      AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

      ++numoutputws;

      g_log.debug() << "Created output Workspace of group = " << wsgroup
                    << "  Property Name = " << propertynamess.str()
                    << " Workspace name = " << wsname.str()
                    << " with Number of events = " << optws->getNumberEvents()
                    << "\n";

      // Update progress report
      m_progress = 0.1 + 0.1 * wsgindex / numnewws;
      progress(m_progress, "Creating output workspace");
      wsgindex += 1.;
    } // If add workspace to output

  } // ENDFOR

  // Set output and do debug report
  setProperty("NumberOutputWS", numoutputws);

  g_log.information("Output workspaces are created. ");
}

//----------------------------------------------------------------------------------------------
/** Create output EventWorkspaces in the case that the splitters are given by
 * MatrixWorkspace
 * Here is the list of class variables that will be updated:
 * - m_outputWorkspacesMap: use (integer) group index to find output
 * EventWorkspace
 * - m_wsNames: vector of output workspaces
 * @brief FilterEvents::createOutputWorkspacesMatrixCase
 */
void FilterEvents::createOutputWorkspacesMatrixCase() {
  // check condition
  if (!m_matrixSplitterWS) {
    g_log.error("createOutputWorkspacesMatrixCase() is applied to "
                "MatrixWorkspace splitters only!");
    throw std::runtime_error("Wrong call!");
  }

  // set up new workspaces
  // Note: m_targetWorkspaceIndexSet is used in different manner among
  // SplittersWorkspace, MatrixWorkspace and TableWorkspace cases
  size_t numoutputws = m_targetWorkspaceIndexSet.size();
  size_t wsgindex = 0;

  for (auto const wsgroup : m_targetWorkspaceIndexSet) {
    if (wsgroup < 0)
      throw std::runtime_error("It is not possible to have split-target group "
                               "index < 0 in MatrixWorkspace case.");

    // workspace name
    std::stringstream wsname;
    if (wsgroup > 0) {
      //  std::string target_name = m_wsGroupIndexTargetMap[wsgroup];
      int target_name = m_wsGroupdYMap[wsgroup];
      wsname << m_outputWSNameBase << "_" << target_name;
    } else {
      wsname << m_outputWSNameBase << "_unfiltered";
    }

    // create new workspace from input EventWorkspace and all the sample logs
    // are copied to the new one
    boost::shared_ptr<EventWorkspace> optws =
        create<DataObjects::EventWorkspace>(*m_eventWS);
    m_outputWorkspacesMap.emplace(wsgroup, optws);

    // add to output workspace property
    std::stringstream propertynamess;
    if (wsgroup == 0) {
      propertynamess << "OutputWorkspace_unfiltered";
    } else {
      propertynamess << "OutputWorkspace_" << wsgroup;
    }

    // Inserted this pair to map
    m_wsNames.push_back(wsname.str());

    // Set (property) to output workspace and set to ADS
    declareProperty(Kernel::make_unique<
                        API::WorkspaceProperty<DataObjects::EventWorkspace>>(
                        propertynamess.str(), wsname.str(), Direction::Output),
                    "Output");
    setProperty(propertynamess.str(), optws);
    AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

    g_log.debug() << "Created output Workspace of group = " << wsgroup
                  << "  Property Name = " << propertynamess.str()
                  << " Workspace name = " << wsname.str()
                  << " with Number of events = " << optws->getNumberEvents()
                  << "\n";

    // Update progress report
    m_progress =
        0.1 +
        0.1 * static_cast<double>(wsgindex) / static_cast<double>(numoutputws);
    progress(m_progress, "Creating output workspace");
    wsgindex += 1;
  } // END-FOR (wsgroup)

  // Set output and do debug report
  g_log.debug() << "Output workspace number: " << numoutputws << "\n";
  setProperty("NumberOutputWS", static_cast<int>(numoutputws));

  return;
}

//----------------------------------------------------------------------------------------------
/** Create output EventWorkspaces in the case that the splitters are given by
 * TableWorkspace
 * Here is the list of class variables that will be updated:
 * - m_outputWorkspacesMap: use (integer) group index to find output
 * EventWorkspace
 * - m_wsNames: vector of output workspaces
 * @brief FilterEvents::createOutputWorkspacesMatrixCase
 */
void FilterEvents::createOutputWorkspacesTableSplitterCase() {
  // check condition
  if (!m_useArbTableSplitters) {
    g_log.error("createOutputWorkspacesTableSplitterCase() is applied to "
                "TableWorkspace splitters only!");
    throw std::runtime_error("Wrong call!");
  }

  // set up new workspaces
  size_t numoutputws = m_targetWorkspaceIndexSet.size();
  size_t wsgindex = 0;

  for (auto const wsgroup : m_targetWorkspaceIndexSet) {
    if (wsgroup < 0)
      throw std::runtime_error("It is not possible to have split-target group "
                               "index < 0 in TableWorkspace case.");

    // workspace name
    std::stringstream wsname;
    if (wsgroup > 0) {
      // get target name via map
      std::string target_name = m_wsGroupIndexTargetMap[wsgroup];
      wsname << m_outputWSNameBase << "_" << target_name;
    } else {
      wsname << m_outputWSNameBase << "_unfiltered";
    }

    // create new workspace
    boost::shared_ptr<EventWorkspace> optws =
        create<DataObjects::EventWorkspace>(*m_eventWS);
    m_outputWorkspacesMap.emplace(wsgroup, optws);

    // add to output workspace property
    std::stringstream propertynamess;
    if (wsgroup < 0) {
      propertynamess << "OutputWorkspace_unfiltered";
    } else {
      propertynamess << "OutputWorkspace_" << wsgroup;
    }

    // Inserted this pair to map
    m_wsNames.push_back(wsname.str());

    // Set (property) to output workspace and set to ADS
    declareProperty(Kernel::make_unique<
                        API::WorkspaceProperty<DataObjects::EventWorkspace>>(
                        propertynamess.str(), wsname.str(), Direction::Output),
                    "Output");
    setProperty(propertynamess.str(), optws);
    AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

    g_log.debug() << "Created output Workspace of group = " << wsgroup
                  << "  Property Name = " << propertynamess.str()
                  << " Workspace name = " << wsname.str()
                  << " with Number of events = " << optws->getNumberEvents()
                  << "\n";

    // Update progress report
    m_progress =
        0.1 +
        0.1 * static_cast<double>(wsgindex) / static_cast<double>(numoutputws);
    progress(m_progress, "Creating output workspace");
    wsgindex += 1;
  } // END-FOR (wsgroup)

  // Set output and do debug report
  g_log.debug() << "Output workspace number: " << numoutputws << "\n";
  setProperty("NumberOutputWS", static_cast<int>(numoutputws));

  return;
}

/** Set up neutron event's TOF correction.
  * It can be (1) parsed from TOF-correction table workspace to vectors,
  * (2) created according to detector's position in instrument;
  * (3) or no correction,i.e., correction value is equal to 1.
  * Offset should be as F*TOF + B
  */
void FilterEvents::setupDetectorTOFCalibration() {
  // Set output correction workspace and set to output
  const size_t numhist = m_eventWS->getNumberHistograms();
  MatrixWorkspace_sptr corrws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numhist, 2, 2));
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
  double ei = 0.;
  if (m_eventWS->run().hasProperty("Ei")) {
    Kernel::Property *eiprop = m_eventWS->run().getProperty("Ei");
    ei = boost::lexical_cast<double>(eiprop->value());
    g_log.debug() << "Using stored Ei value " << ei << "\n";
  } else {
    ei = getProperty("IncidentEnergy");
    if (isEmpty(ei))
      throw std::invalid_argument(
          "No Ei value has been set or stored within the run information.");
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
  vector<string> colnames = m_detCorrectWorkspace->getColumnNames();
  bool hasshift = false;
  if (colnames.size() < 2)
    throw runtime_error("Input table workspace is not valide.");
  else if (colnames.size() >= 3)
    hasshift = true;

  bool usedetid;
  if (colnames[0].compare("DetectorID") == 0)
    usedetid = true;
  else if (colnames[0].compare("Spectrum") == 0)
    usedetid = false;
  else {
    usedetid = false;
    stringstream errss;
    errss << "First column must be either DetectorID or Spectrum. "
          << colnames[0] << " is not supported. ";
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
      errss << "Correction (i.e., offset) equal to " << offset_factor
            << " of row "
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
    g_log.warning() << "Input correction table workspace has more detectors ("
                    << toffactormap.size() << ") than input workspace "
                    << m_eventWS->getName() << "'s spectra number (" << numhist
                    << ".\n";
  } else if (toffactormap.size() < numhist) {
    stringstream errss;
    errss << "Input correction table workspace has more detectors ("
          << toffactormap.size() << ") than input workspace "
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
      auto detids = events.getDetectorIDs();
      if (detids.size() != 1) {
        // Check whether there are more than 1 detector per spectra.
        stringstream errss;
        errss << "The assumption is that one spectrum has one and only one "
                 "detector. "
              << "Error is found at spectrum " << i << ".  It has "
              << detids.size() << " detectors.";
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
              << "w/ ID << " << detid << " of spectrum " << i
              << " in Eventworkspace " << m_eventWS->getName()
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
      size_t wsindex = static_cast<size_t>(fiter->first);
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
      size_t wsindex = static_cast<size_t>(fiter->first);
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

/** Main filtering method
  * Structure: per spectrum --> per workspace
 */
void FilterEvents::filterEventsBySplitters(double progressamount) {
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();

  // Loop over the histograms (detector spectra) to do split from 1 event list
  // to N event list
  g_log.debug() << "Number of spectra in input/source EventWorkspace = "
                << numberOfSpectra << ".\n";

  // FIXME - Turn on parallel:
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws) {
    PARALLEL_START_INTERUPT_REGION

    // Filter the non-skipped
    if (!m_vecSkip[iws]) {
      // Get the output event lists (should be empty) to be a map
      std::map<int, DataObjects::EventList *> outputs;
      PARALLEL_CRITICAL(build_elist) {
        for (auto &ws : m_outputWorkspacesMap) {
          int index = ws.first;
          auto &output_el = ws.second->getSpectrum(iws);
          outputs.emplace(index, &output_el);
        }
      }
      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList &input_el = m_eventWS->getSpectrum(iws);

      // Perform the filtering (using the splitting function and just one
      // output)
      if (m_filterByPulseTime) {
        input_el.splitByPulseTime(m_splitters, outputs);
      } else if (m_tofCorrType != NoneCorrect) {
        input_el.splitByFullTime(m_splitters, outputs, true,
                                 m_detTofFactors[iws], m_detTofOffsets[iws]);
      } else {
        input_el.splitByFullTime(m_splitters, outputs, false, 1.0, 0.0);
      }
    }

    PARALLEL_END_INTERUPT_REGION
  } // END FOR i = 0
  PARALLEL_CHECK_INTERUPT_REGION

  // Split the sample logs in each target workspace.
  progress(0.1 + progressamount, "Splitting logs");

  if (!m_splitSampleLogs) {
    // Skip if choice is no
    g_log.notice("Sample logs are not split by user's choice.");
    return;
  }

  auto lognames = this->getTimeSeriesLogNames();
  g_log.debug() << "[FilterEvents D1214]:  Number of TimeSeries Logs = "
                << lognames.size() << " to " << m_outputWorkspacesMap.size()
                << " outptu workspaces. \n";

  double numws = static_cast<double>(m_outputWorkspacesMap.size());
  double outwsindex = 0.;

  // split sample logs from original workspace to new one
  for (auto &ws : m_outputWorkspacesMap) {
    int wsindex = ws.first;
    DataObjects::EventWorkspace_sptr opws = ws.second;

    // Generate a list of splitters for current output workspace
    Kernel::TimeSplitterType splitters = generateSplitters(wsindex);

    g_log.debug() << "[FilterEvents D1215]: Output workspace Index " << wsindex
                  << ": Name = " << opws->getName()
                  << "; Number of splitters = " << splitters.size() << ".\n";

    // Skip output workspace has ZERO splitters
    if (splitters.empty()) {
      g_log.warning() << "[FilterEvents] Workspace " << opws->getName()
                      << " Indexed @ " << wsindex
                      << " won't have logs splitted due to zero splitter size. "
                      << ".\n";
      continue;
    }

    // Split log
    // FIXME-TODO: SHALL WE MOVE THIS PART OUTSIDE OF THIS METHOD?
    size_t numlogs = lognames.size();
    for (size_t ilog = 0; ilog < numlogs; ++ilog) {
      this->splitLog(opws, lognames[ilog], splitters);
    }
    opws->mutableRun().integrateProtonCharge();

    progress(0.1 + progressamount + outwsindex / numws * 0.2, "Splitting logs");
    outwsindex += 1.;
  }
}

/** Split events by splitters represented by vector
  */
void FilterEvents::filterEventsByVectorSplitters(double progressamount) {
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  // FIXME : consider to use vector to index workspace and event list

  // Loop over the histograms (detector spectra) to do split from 1 event list
  // to N event list
  g_log.notice() << "Filter by vector splitters: Number of spectra in "
                    "input/source EventWorkspace = " << numberOfSpectra
                 << ".\n";

  if (m_filterByPulseTime) {
    size_t num_proton_charges =
        m_eventWS->run().getProperty("proton_charge")->size();
    if (num_proton_charges < m_vecSplitterTime.size())
      throw runtime_error("It is not a good practice to split fast event by "
                          "pulse time when there are more splitters than pulse "
                          "times.");
    else
      g_log.warning("User should understand the inaccurancy to filter events "
                    "by pulse time.");
  }

  /*
  for (size_t i = 0; i < m_vecSplitterGroup.size(); ++i)
    std::cout << "splitter " << i << ": " << m_vecSplitterTime[i] << ", "
              << m_vecSplitterGroup[i] << "\n";
  */

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws) {
    PARALLEL_START_INTERUPT_REGION

    // Filter the non-skipped spectrum
    if (!m_vecSkip[iws]) {
      // Get the output event lists (should be empty) to be a map
      map<int, DataObjects::EventList *> outputs;
      PARALLEL_CRITICAL(build_elist) {
        for (auto &ws : m_outputWorkspacesMap) {
          int index = ws.first;
          auto &output_el = ws.second->getSpectrum(iws);
          outputs.emplace(index, &output_el);
        }
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList &input_el = m_eventWS->getSpectrum(iws);

      bool printdetail = false;
      if (m_useDBSpectrum)
        printdetail = (iws == static_cast<int64_t>(m_dbWSIndex));

      // Perform the filtering (using the splitting function and just one
      // output)
      std::string logmessage;
      if (m_tofCorrType != NoneCorrect) {
        logmessage = input_el.splitByFullTimeMatrixSplitter(
            m_vecSplitterTime, m_vecSplitterGroup, outputs, true,
            m_detTofFactors[iws], m_detTofOffsets[iws]);
      } else {
        logmessage = input_el.splitByFullTimeMatrixSplitter(
            m_vecSplitterTime, m_vecSplitterGroup, outputs, false, 1.0, 0.0);
      }

      if (printdetail)
        g_log.notice(logmessage);
    }

    PARALLEL_END_INTERUPT_REGION
  } // END FOR i = 0
  PARALLEL_CHECK_INTERUPT_REGION

  // Finish (1) adding events and splitting the sample logs in each target
  // workspace.
  progress(0.1 + progressamount, "Splitting logs");

  g_log.notice("Splitters in format of Matrixworkspace are not recommended to "
               "split sample logs. ");

  // split sample logs

  // find the maximum index of the outputs' index
  std::set<int>::iterator target_iter;
  int max_target_index = 0;
  for (target_iter = m_targetWorkspaceIndexSet.begin();
       target_iter != m_targetWorkspaceIndexSet.end(); ++target_iter) {
    if (*target_iter > max_target_index)
      max_target_index = *target_iter;
  }

  // convert vector of int64 to Time
  std::vector<Kernel::DateAndTime> split_datetime_vec(m_vecSplitterTime.size());
  for (size_t i = 0; i < m_vecSplitterTime.size(); ++i) {
    DateAndTime split_time(m_vecSplitterTime[i]);
    split_datetime_vec[i] = split_time;
  }

  for (auto property : m_eventWS->run().getProperties()) {
    // insert 0 even if it is empty for contructing a vector
    g_log.debug() << "Process sample log" << property->name() << "\n";
    TimeSeriesProperty<double> *dbl_prop =
        dynamic_cast<TimeSeriesProperty<double> *>(property);
    TimeSeriesProperty<int> *int_prop =
        dynamic_cast<TimeSeriesProperty<int> *>(property);
    if (dbl_prop) {
      std::vector<TimeSeriesProperty<double> *> output_vector;
      for (int tindex = 0; tindex <= max_target_index; ++tindex) {
        TimeSeriesProperty<double> *new_property =
            new TimeSeriesProperty<double>(dbl_prop->name());
        output_vector.push_back(new_property);
      }

      // split
      dbl_prop->splitByTimeVector(split_datetime_vec, m_vecSplitterGroup,
                                  output_vector);

      // set to output workspace
      for (int tindex = 0; tindex <= max_target_index; ++tindex) {
        // find output workspace
        std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;
        wsiter = m_outputWorkspacesMap.find(tindex);
        if (wsiter == m_outputWorkspacesMap.end()) {
          ;
          //  g_log.error() << "Workspace target (" << tindex
          //          << ") does not have workspace associated."
          //        << "\n";
        } else {
          DataObjects::EventWorkspace_sptr ws_i = wsiter->second;
          ws_i->mutableRun().addProperty(output_vector[tindex], true);
        }
      }

    } else if (int_prop) {
      // integer log
      std::vector<TimeSeriesProperty<int> *> output_vector;
      for (int tindex = 0; tindex <= max_target_index; ++tindex) {
        TimeSeriesProperty<int> *new_property =
            new TimeSeriesProperty<int>(int_prop->name());
        output_vector.push_back(new_property);
      }

      // split
      int_prop->splitByTimeVector(split_datetime_vec, m_vecSplitterGroup,
                                  output_vector);

      // set to output workspace
      for (int tindex = 0; tindex <= max_target_index; ++tindex) {
        // find output workspace
        std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;
        wsiter = m_outputWorkspacesMap.find(tindex);
        if (wsiter == m_outputWorkspacesMap.end()) {
          g_log.error() << "Workspace target (" << tindex
                        << ") does not have workspace associated."
                        << "\n";
        } else {
          DataObjects::EventWorkspace_sptr ws_i = wsiter->second;
          ws_i->mutableRun().addProperty(output_vector[tindex], true);
        }
      }
    } else {
      // TODO:FIXME - Copy the prperty!
      // set to output workspace ??? -- may not be needed! as the way how output
      // workspace is created
    }
  }

  for (int tindex = 0; tindex <= max_target_index; ++tindex) {
    // set to output workspace
    for (int tindex = 0; tindex <= max_target_index; ++tindex) {
      // find output workspace
      std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;
      wsiter = m_outputWorkspacesMap.find(tindex);
      if (wsiter == m_outputWorkspacesMap.end()) {
        g_log.error() << "Workspace target (" << tindex
                      << ") does not have workspace associated."
                      << "\n";
      } else {
        DataObjects::EventWorkspace_sptr ws_i = wsiter->second;
        ws_i->mutableRun().integrateProtonCharge();
      }
    }
  }

  return;
}

/** Generate splitters for specified workspace index as a subset of
 * m_splitters
 */
Kernel::TimeSplitterType FilterEvents::generateSplitters(int wsindex) {
  Kernel::TimeSplitterType splitters;
  for (const auto &splitter : m_splitters) {
    int index = splitter.index();
    if (index == wsindex) {
      splitters.push_back(splitter);
    }
  }
  return splitters;
}

/** Split a log by splitters
 */
void FilterEvents::splitLog(EventWorkspace_sptr eventws, std::string logname,
                            TimeSplitterType &splitters) {
  // cast property to both double TimeSeriesProperty and IntSeriesProperty
  Kernel::TimeSeriesProperty<double> *dbl_prop =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          eventws->mutableRun().getProperty(logname));
  Kernel::TimeSeriesProperty<int> *int_prop =
      dynamic_cast<Kernel::TimeSeriesProperty<int> *>(
          eventws->mutableRun().getProperty(logname));

  if (!dbl_prop && !int_prop) {
    std::stringstream errmsg;
    errmsg << "Log " << logname
           << " is not TimeSeriesProperty<int> or TimeSeriesProperty<double>. "
           << "Unable to split.";
    throw std::runtime_error(errmsg.str());
  } else {
    for (const auto &split : splitters) {
      g_log.debug() << "Workspace " << eventws->getName() << ": "
                    << "log name = " << logname
                    << ", duration = " << split.duration() << " from "
                    << split.start() << " to " << split.stop() << ".\n";
    }

    // split log
    if (dbl_prop)
      dbl_prop->filterByTimes(splitters);
    else
      int_prop->filterByTimes(splitters);
  }
}

/** Generate a vector of integer time series property for each splitter
 * corresponding to each target (in integer)
 * in each splitter-time-series-property, 1 stands for include and 0 stands for
 * time for neutrons to be discarded.
 * If there is no UN-DEFINED
 * @brief FilterEvents::generateSplitterTSP
 * @param split_tsp_vec
 */
void FilterEvents::generateSplitterTSP(
    std::vector<Kernel::TimeSeriesProperty<int> *> &split_tsp_vec) {
  // clear vector to set up
  split_tsp_vec.clear();

  // initialize m_maxTargetIndex + 1 time series properties in integer
  for (int itarget = 0; itarget <= m_maxTargetIndex; ++itarget) {
    Kernel::TimeSeriesProperty<int> *split_tsp =
        new Kernel::TimeSeriesProperty<int>("splitter");
    split_tsp_vec.push_back(split_tsp);
    // add initial value
    split_tsp->addValue(Kernel::DateAndTime(m_runStartTime), 0);
  }

  // start to go through  m_vecSplitterTime (int64) and m_vecSplitterGroup add
  // each entry to corresponding splitter TSP
  for (size_t igrp = 0; igrp < m_vecSplitterGroup.size(); ++igrp) {
    int itarget = m_vecSplitterGroup[igrp];
    DateAndTime start_time(m_vecSplitterTime[igrp]);
    if (start_time <= m_runStartTime) {
      // clear the initial value with check first
      if (split_tsp_vec[itarget]->size() != 1) {
        g_log.error() << "With start time " << start_time
                      << " same as run start time " << m_runStartTime
                      << ", the TSP must have only 1 entry from "
                         "initialization.  But not it has "
                      << split_tsp_vec[itarget]->size() << "entries\n";
        throw std::runtime_error("Coding logic error");
      }
      split_tsp_vec[itarget]->clear();
    }
    DateAndTime stop_time(m_vecSplitterTime[igrp + 1]);
    split_tsp_vec[itarget]->addValue(start_time, 1);
    split_tsp_vec[itarget]->addValue(stop_time, 0);
  }

  return;
}

/** Generate the splitter's time series property (log) the splitters workspace
 * @brief FilterEvents::generateSplitterTSPalpha
 * @param split_tsp_vec
 */
void FilterEvents::generateSplitterTSPalpha(
    std::vector<Kernel::TimeSeriesProperty<int> *> &split_tsp_vec) {
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
    Kernel::TimeSeriesProperty<int> *split_tsp =
        new Kernel::TimeSeriesProperty<int>("splitter");
    split_tsp->addValue(m_runStartTime, 0);
    split_tsp_vec.push_back(split_tsp);
  }

  for (SplittingInterval splitter : m_splitters) {
    int itarget = splitter.index();
    if (itarget >= static_cast<int>(split_tsp_vec.size()))
      throw std::runtime_error("Target workspace index is out of range!");

    if (splitter.start() == m_runStartTime) {
      // there should be only 1 value in the splitter and clear it.
      if (split_tsp_vec[itarget]->size() != 1) {
        throw std::runtime_error(
            "Splitter must have 1 value with initialization.");
      }
      split_tsp_vec[itarget]->clear();
    }
    split_tsp_vec[itarget]->addValue(splitter.start(), 1);
    split_tsp_vec[itarget]->addValue(splitter.stop(), 0);
  }

  return;
}

/** add the splitter TimeSeriesProperty logs to each workspace
 * @brief FilterEvents::mapSplitterTSPtoWorkspaces
 * @param split_tsp_vec
 */
void FilterEvents::mapSplitterTSPtoWorkspaces(
    const std::vector<Kernel::TimeSeriesProperty<int> *> &split_tsp_vec) {
  if (m_useSplittersWorkspace) {
    g_log.debug() << "There are " << split_tsp_vec.size()
                  << " TimeSeriesPropeties.\n";
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator miter;
    for (miter = m_outputWorkspacesMap.begin();
         miter != m_outputWorkspacesMap.end(); ++miter) {
      g_log.debug() << "Output workspace index: " << miter->first << "\n";
      if (0 <= miter->first &&
          miter->first < static_cast<int>(split_tsp_vec.size())) {
        DataObjects::EventWorkspace_sptr outws = miter->second;
        outws->mutableRun().addProperty(split_tsp_vec[miter->first]);
      }
    }
  } else {
    // Either Table-type or Matrix-type splitters
    for (int itarget = 0; itarget < static_cast<int>(split_tsp_vec.size());
         ++itarget) {
      // use itarget to find the workspace that is mapped
      std::map<int, DataObjects::EventWorkspace_sptr>::iterator ws_iter;
      ws_iter = m_outputWorkspacesMap.find(itarget);

      // skip if an itarget does not have matched workspace
      if (ws_iter == m_outputWorkspacesMap.end()) {
        g_log.warning() << "iTarget " << itarget
                        << " does not have any workspace associated.\n";
        continue;
      }

      // get the workspace and add property
      DataObjects::EventWorkspace_sptr outws = ws_iter->second;
      outws->mutableRun().addProperty(split_tsp_vec[itarget]);
    }

  } // END-IF-ELSE (splitter-type)

  return;
}

/** Get all filterable logs' names (double and integer)
 * @returns Vector of names of logs
 */
std::vector<std::string> FilterEvents::getTimeSeriesLogNames() {
  std::vector<std::string> lognames;

  const std::vector<Kernel::Property *> allprop =
      m_eventWS->mutableRun().getProperties();
  for (auto ip : allprop) {
    // cast to double log and integer log
    Kernel::TimeSeriesProperty<double> *dbltimeprop =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(ip);
    Kernel::TimeSeriesProperty<int> *inttimeprop =
        dynamic_cast<Kernel::TimeSeriesProperty<int> *>(ip);

    // append to vector if it is either double TimeSeries or int TimeSeries
    if (dbltimeprop || inttimeprop) {
      std::string pname = ip->name();
      lognames.push_back(pname);
    }
  }

  return lognames;
}

} // namespace Mantid
} // namespace Algorithms
