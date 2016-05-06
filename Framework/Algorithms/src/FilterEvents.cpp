#include "MantidAlgorithms/FilterEvents.h"
#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"
#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ArrayProperty.h"
#include <memory>
#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(FilterEvents)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FilterEvents::FilterEvents()
    : m_eventWS(), m_splittersWorkspace(), m_matrixSplitterWS(),
      m_detCorrectWorkspace(), m_useTableSplitters(false), m_workGroupIndexes(),
      m_splitters(), m_outputWS(), m_wsNames(), m_detTofOffsets(),
      m_detTofFactors(), m_FilterByPulseTime(false), m_informationWS(),
      m_hasInfoWS(), m_progress(0.), m_outputWSNameBase(), m_toGroupWS(false),
      m_vecSplitterTime(), m_vecSplitterGroup(), m_splitSampleLogs(false),
      m_useDBSpectrum(false), m_dbWSIndex(-1), m_tofCorrType(),
      m_specSkipType(), m_vecSkip(), m_isSplittersRelativeTime(false),
      m_filterStartTime(0) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FilterEvents::~FilterEvents() {}

//----------------------------------------------------------------------------------------------
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

  return;
}

//----------------------------------------------------------------------------------------------
/** Execution body
 */
void FilterEvents::exec() {
  // Process algorithm properties
  processAlgorithmProperties();

  // Examine workspace for detectors
  examineEventWS();

  // Parse splitters
  m_progress = 0.0;
  progress(m_progress, "Processing SplittersWorkspace.");
  if (m_useTableSplitters)
    processSplittersWorkspace();
  else
    processMatrixSplitterWorkspace();

  // Create output workspaces
  m_progress = 0.1;
  progress(m_progress, "Create Output Workspaces.");
  createOutputWorkspaces();

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
  if (m_useTableSplitters)
    filterEventsBySplitters(progressamount);
  else
    filterEventsByVectorSplitters(progressamount);

  // Optional to group detector
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
      g_log.error() << "Grouping all output workspaces fails." << std::endl;
    }
  }

  // Form the names of output workspaces
  std::vector<std::string> outputwsnames;
  std::map<int, DataObjects::EventWorkspace_sptr>::iterator miter;
  for (miter = m_outputWS.begin(); miter != m_outputWS.end(); ++miter) {
    outputwsnames.push_back(miter->second->name());
  }
  setProperty("OutputWorkspaceNames", outputwsnames);

  m_progress = 1.0;
  progress(m_progress, "Completed");

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

  // Process splitting workspace (table or data)
  API::Workspace_sptr tempws = this->getProperty("SplitterWorkspace");

  m_splittersWorkspace =
      boost::dynamic_pointer_cast<SplittersWorkspace>(tempws);
  if (m_splittersWorkspace) {
    m_useTableSplitters = true;
  } else {
    m_matrixSplitterWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tempws);
    if (m_matrixSplitterWS) {
      m_useTableSplitters = false;
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
  m_FilterByPulseTime = this->getProperty("FilterByPulseTime");

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

  // Splitters are given relative time
  m_isSplittersRelativeTime = getProperty("RelativeTime");
  if (m_isSplittersRelativeTime) {
    // Using relative time
    std::string start_time_str = getProperty("FilterStartTime");
    if (start_time_str.size() > 0) {
      // User specifies the filter starting time
      Kernel::DateAndTime temp_shift_time(start_time_str);
      m_filterStartTime = temp_shift_time;
    } else {
      // Retrieve filter starting time from property run_start as default
      if (m_eventWS->run().hasProperty("run_start")) {
        Kernel::DateAndTime temp_shift_time(
            m_eventWS->run().getProperty("run_start")->value());
        m_filterStartTime = temp_shift_time;
      } else {
        throw std::runtime_error(
            "Input event workspace does not have property run_start. "
            "User does not specifiy filter start time."
            "Splitters cannot be in reltive time.");
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Examine whether any spectrum does not have detector
  */
void FilterEvents::examineEventWS() {
  size_t numhist = m_eventWS->getNumberHistograms();
  m_vecSkip.resize(numhist, false);

  if (m_specSkipType == EventFilterSkipNoDetTOFCorr &&
      m_tofCorrType == NoneCorrect) {
    // No TOF correction and skip spectrum only if TOF correction is required
    g_log.notice("By user's choice, No spectrum will be skipped even if it has "
                 "no detector.");
  } else {
    stringstream msgss;
    size_t numskipspec = 0;
    size_t numeventsskip = 0;

    for (size_t i = 0; i < numhist; ++i) {
      bool skip = false;

      // Access detector of the spectrum
      try {
        IDetector_const_sptr tempdet = m_eventWS->getDetector(i);
        if (!tempdet)
          skip = true;
      } catch (const Kernel::Exception::NotFoundError &) {
        // No detector found
        skip = true;
      }

      // Output
      if (skip) {
        m_vecSkip[i] = true;

        ++numskipspec;
        const EventList &elist = m_eventWS->getEventList(i);
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

  return;
}

//----------------------------------------------------------------------------------------------
/** Purpose:
 *    Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
 *    and create a map for all workspace group number
 *  Requirements:
 *  Gaurantees
 * @brief FilterEvents::processSplittersWorkspace
 */
void FilterEvents::processSplittersWorkspace() {
  // 1. Init data structure
  size_t numsplitters = m_splittersWorkspace->getNumberSplitters();
  m_splitters.reserve(numsplitters);

  // 2. Insert all splitters
  bool inorder = true;
  for (size_t i = 0; i < numsplitters; i++) {
    m_splitters.push_back(m_splittersWorkspace->getSplitter(i));
    m_workGroupIndexes.insert(m_splitters.back().index());
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
  m_workGroupIndexes.insert(-1);

  // 5. Add information
  if (m_hasInfoWS) {
    if (m_workGroupIndexes.size() > m_informationWS->rowCount() + 1) {
      g_log.warning() << "Input Splitters Workspace has different entries ("
                      << m_workGroupIndexes.size() - 1
                      << ") than input information workspaces ("
                      << m_informationWS->rowCount() << "). "
                      << "  Information may not be accurate. " << std::endl;
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief FilterEvents::processMatrixSplitterWorkspace
 * Purpose:
 *   Convert the splitters in matrix workspace to a vector of splitters
 * Requirements:
 *   m_matrixSplitterWS has valid value
 *   vecX's size must be one larger than and that of vecY of m_matrixSplitterWS
 * Guarantees
 *   Splitters stored in m_matrixSpliterWS are transformed to
 *   m_vecSplitterTime and m_workGroupIndexes, which are of same size
 */
void FilterEvents::processMatrixSplitterWorkspace() {
  // Check input workspace validity
  assert(m_matrixSplitterWS);

  const MantidVec &vecX = m_matrixSplitterWS->readX(0);
  const MantidVec &vecY = m_matrixSplitterWS->readY(0);
  size_t sizex = vecX.size();
  size_t sizey = vecY.size();
  assert(sizex - sizey == 1);

  // Assign vectors for time comparison
  m_vecSplitterTime.assign(vecX.size(), 0);
  m_vecSplitterGroup.assign(vecY.size(), -1);

  // Transform vector
  for (size_t i = 0; i < sizex; ++i) {
    m_vecSplitterTime[i] = static_cast<int64_t>(vecX[i]);
  }
  // shift the splitters' time if applied
  if (m_isSplittersRelativeTime) {
    int64_t time_shift_ns = m_filterStartTime.totalNanoseconds();
    for (size_t i = 0; i < sizex; ++i)
      m_vecSplitterTime[i] += time_shift_ns;
  }

  for (size_t i = 0; i < sizey; ++i) {
    m_vecSplitterGroup[i] = static_cast<int>(vecY[i]);
    m_workGroupIndexes.insert(m_vecSplitterGroup[i]);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Create a list of EventWorkspace for output
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
  for (auto wsgroup : m_workGroupIndexes) {
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
  double numnewws = static_cast<double>(m_workGroupIndexes.size());
  double wsgindex = 0.;

  for (auto const wsgroup : m_workGroupIndexes) {
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

    // Generate one of the output workspaces & Copy geometry over. But we
    // don't
    // copy the data.
    DataObjects::EventWorkspace_sptr optws =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            API::WorkspaceFactory::Instance().create(
                "EventWorkspace", m_eventWS->getNumberHistograms(), 2, 1));
    API::WorkspaceFactory::Instance().initializeFromParent(m_eventWS, optws,
                                                           false);
    m_outputWS.emplace(wsgroup, optws);

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
    }

    // Add to output properties.  There shouldn't be any workspace
    // (non-unfiltered) skipped from group index
    if (add2output) {
      // Generate output property name
      std::stringstream propertynamess;
      propertynamess << "OutputWorkspace_" << wsgroup;

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

  return;
}

//----------------------------------------------------------------------------------------------
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

        corrws->dataY(i)[0] = correction.factor;
        corrws->dataY(i)[1] = correction.offset;
      }
    }
  }

  return;
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

//----------------------------------------------------------------------------------------------
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
                    << m_eventWS->name() << "'s spectra number (" << numhist
                    << ".\n";
  } else if (toffactormap.size() < numhist) {
    stringstream errss;
    errss << "Input correction table workspace has more detectors ("
          << toffactormap.size() << ") than input workspace "
          << m_eventWS->name() << "'s spectra number (" << numhist << ".\n";
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
      const DataObjects::EventList events = m_eventWS->getEventList(i);
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
              << " in Eventworkspace " << m_eventWS->name()
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

  return;
}

//----------------------------------------------------------------------------------------------
/** Main filtering method
  * Structure: per spectrum --> per workspace
 */
void FilterEvents::filterEventsBySplitters(double progressamount) {
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

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
        for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end();
             ++wsiter) {
          int index = wsiter->first;
          DataObjects::EventList *output_el =
              wsiter->second->getEventListPtr(iws);
          outputs.emplace(index, output_el);
        }
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList &input_el = m_eventWS->getEventList(iws);

      // Perform the filtering (using the splitting function and just one
      // output)
      if (m_FilterByPulseTime) {
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
                << lognames.size() << " to " << m_outputWS.size()
                << " outptu workspaces. \n";

  double numws = static_cast<double>(m_outputWS.size());
  double outwsindex = 0.;
  for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++wsiter) {
    int wsindex = wsiter->first;
    DataObjects::EventWorkspace_sptr opws = wsiter->second;

    // Generate a list of splitters for current output workspace
    Kernel::TimeSplitterType splitters = generateSplitters(wsindex);

    g_log.debug() << "[FilterEvents D1215]: Output workspace Index " << wsindex
                  << ": Name = " << opws->name()
                  << "; Number of splitters = " << splitters.size() << ".\n";

    // Skip output workspace has ZERO splitters
    if (splitters.empty()) {
      g_log.warning() << "[FilterEvents] Workspace " << opws->name()
                      << " Indexed @ " << wsindex
                      << " won't have logs splitted due to zero splitter size. "
                      << ".\n";
      continue;
    }

    // Split log
    size_t numlogs = lognames.size();
    for (size_t ilog = 0; ilog < numlogs; ++ilog) {
      this->splitLog(opws, lognames[ilog], splitters);
    }
    opws->mutableRun().integrateProtonCharge();

    progress(0.1 + progressamount + outwsindex / numws * 0.2, "Splitting logs");
    outwsindex += 1.;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Split events by splitters represented by vector
  */
void FilterEvents::filterEventsByVectorSplitters(double progressamount) {
  size_t numberOfSpectra = m_eventWS->getNumberHistograms();
  // FIXME : consider to use vector to index workspace and event list
  std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

  // Loop over the histograms (detector spectra) to do split from 1 event list
  // to N event list
  g_log.debug() << "Number of spectra in input/source EventWorkspace = "
                << numberOfSpectra << ".\n";

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws) {
    PARALLEL_START_INTERUPT_REGION

    // Filter the non-skipped spectrum
    if (!m_vecSkip[iws]) {
      // Get the output event lists (should be empty) to be a map
      map<int, DataObjects::EventList *> outputs;
      PARALLEL_CRITICAL(build_elist) {
        for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end();
             ++wsiter) {
          int index = wsiter->first;
          DataObjects::EventList *output_el =
              wsiter->second->getEventListPtr(iws);
          outputs.emplace(index, output_el);
        }
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList &input_el = m_eventWS->getEventList(iws);

      bool printdetail = false;
      if (m_useDBSpectrum)
        printdetail = (iws == static_cast<int64_t>(m_dbWSIndex));

      // Perform the filtering (using the splitting function and just one
      // output)
      std::string logmessage("");
      if (m_FilterByPulseTime) {
        throw runtime_error(
            "It is not a good practice to split fast event by pulse time. ");
      } else if (m_tofCorrType != NoneCorrect) {
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

  return;
}

//----------------------------------------------------------------------------------------------
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

//----------------------------------------------------------------------------------------------
/** Split a log by splitters
 */
void FilterEvents::splitLog(EventWorkspace_sptr eventws, std::string logname,
                            TimeSplitterType &splitters) {
  Kernel::TimeSeriesProperty<double> *prop =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          eventws->mutableRun().getProperty(logname));
  if (!prop) {
    g_log.warning() << "Log " << logname
                    << " is not TimeSeriesProperty.  Unable to split."
                    << std::endl;
    return;
  } else {
    for (const auto &split : splitters) {
      g_log.debug() << "[FilterEvents DB1226] Going to filter workspace "
                    << eventws->name() << ": "
                    << "log name = " << logname
                    << ", duration = " << split.duration() << " from "
                    << split.start() << " to " << split.stop() << ".\n";
    }
  }

  prop->filterByTimes(splitters);

  return;
}

//----------------------------------------------------------------------------------------------
/** Get all filterable logs' names
 * @returns Vector of names of logs
 */
std::vector<std::string> FilterEvents::getTimeSeriesLogNames() {
  std::vector<std::string> lognames;

  const std::vector<Kernel::Property *> allprop =
      m_eventWS->mutableRun().getProperties();
  for (auto ip : allprop) {
    Kernel::TimeSeriesProperty<double> *timeprop =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(ip);
    if (timeprop) {
      std::string pname = timeprop->name();
      lognames.push_back(pname);
    }
  }

  return lognames;
}

} // namespace Mantid
} // namespace Algorithms
