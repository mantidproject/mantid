#include "MantidAlgorithms/FilterEvents.h"
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
FilterEvents::FilterEvents() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FilterEvents::~FilterEvents() {}

//----------------------------------------------------------------------------------------------
/** Declare Inputs
 */
void FilterEvents::init() {
  declareProperty(new API::WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(new API::WorkspaceProperty<API::Workspace>(
                      "SplitterWorkspace", "", Direction::Input),
                  "An input SpilltersWorskpace for filtering");

  declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                  "The base name to use for the output workspace");

  declareProperty(
      new WorkspaceProperty<TableWorkspace>(
          "InformationWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Optional output for the information of each splitter workspace index.");

  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("OutputTOFCorrectionWorkspace",
                                             "TOFCorrectWS", Direction::Output),
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
  vector<string> corrtypes;
  corrtypes.push_back("None");
  corrtypes.push_back("Customized");
  corrtypes.push_back("Direct");
  corrtypes.push_back("Elastic");
  corrtypes.push_back("Indirect");
  declareProperty("CorrectionToSample", "None",
                  boost::make_shared<StringListValidator>(corrtypes),
                  "Type of correction on neutron events to sample time from "
                  "detector time. ");

  auto tablewsprop = new WorkspaceProperty<TableWorkspace>(
      "DetectorTOFCorrectionWorkspace", "", Direction::Input,
      PropertyMode::Optional);
  declareProperty(tablewsprop, "Name of table workspace containing the log "
                               "time correction factor for each detector. ");
  setPropertySettings(
      "DetectorTOFCorrectionWorkspace",
      new VisibleWhenProperty("CorrectionToSample", IS_EQUAL_TO, "Customized"));

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("IncidentEnergy", EMPTY_DBL(), mustBePositive,
                  "Value of incident energy (Ei) in meV in direct mode.");
  setPropertySettings(
      "IncidentEnergy",
      new VisibleWhenProperty("CorrectionToSample", IS_EQUAL_TO, "Direct"));

  // Algorithm to spectra without detectors
  vector<string> spec_no_det;
  spec_no_det.push_back("Skip");
  spec_no_det.push_back("Skip only if TOF correction");
  declareProperty("SpectrumWithoutDetector", "Skip",
                  boost::make_shared<StringListValidator>(spec_no_det),
                  "Approach to deal with spectrum without detectors. ");

  declareProperty(
      "SplitSampleLogs", true,
      "If selected, all sample logs will be splitted by the  "
      "event splitters.  It is not recommended for fast event log splitters. ");

  declareProperty("NumberOutputWS", 0,
                  "Number of output output workspace splitted. ",
                  Direction::Output);

  declareProperty("DBSpectrum", EMPTY_INT(),
                  "Spectrum (workspace index) for debug purpose. ");

  declareProperty(
      new ArrayProperty<string>("OutputWorkspaceNames", Direction::Output),
      "List of output workspaces names");

  return;
}

//----------------------------------------------------------------------------------------------
/** Execution body
 */
void FilterEvents::exec() {
  // Process algorithm properties
  processProperties();

  // Examine workspace for detectors
  examineEventWS();

  // Parse splitters
  mProgress = 0.0;
  progress(mProgress, "Processing SplittersWorkspace.");
  if (m_useTableSplitters)
    processSplittersWorkspace();
  else
    processMatrixSplitterWorkspace();

  // Create output workspaces
  mProgress = 0.1;
  progress(mProgress, "Create Output Workspaces.");
  createOutputWorkspaces();

  // Optionall import corrections
  mProgress = 0.20;
  progress(mProgress, "Importing TOF corrections. ");
  setupDetectorTOFCalibration();

  // Filter Events
  mProgress = 0.30;
  progress(mProgress, "Filter Events.");
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
    mProgress = 0.9;
    progress(mProgress, "Group workspaces");

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

  mProgress = 1.0;
  progress(mProgress, "Completed");

  return;
}

//----------------------------------------------------------------------------------------------
/** Process input properties
 */
void FilterEvents::processProperties() {
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
  mFilterByPulseTime = this->getProperty("FilterByPulseTime");

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
/** Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
 *  and create a map for all workspace group number
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
  mProgress = 0.05;
  progress(mProgress);

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
  */
void FilterEvents::processMatrixSplitterWorkspace() {
  // Check input workspace validity
  const MantidVec &vecX = m_matrixSplitterWS->readX(0);
  const MantidVec &vecY = m_matrixSplitterWS->readY(0);
  size_t sizex = vecX.size();
  size_t sizey = vecY.size();
  if (sizex - sizey != 1)
    throw runtime_error("Size must be N and N-1.");

  // Assign vectors for time comparison
  m_vecSplitterTime.assign(vecX.size(), 0);
  m_vecSplitterGroup.assign(vecY.size(), -1);

  // Transform vector
  for (size_t i = 0; i < sizex; ++i) {
    m_vecSplitterTime[i] = static_cast<int64_t>(vecX[i]);
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
      int &indexws = row.Int(0);
      std::string &info = row.String(1);
      infomap.insert(std::make_pair(indexws, info));
    }
  }

  // Determine the minimum group index number
  int minwsgroup = INT_MAX;
  for (set<int>::iterator groupit = m_workGroupIndexes.begin();
       groupit != m_workGroupIndexes.end(); ++groupit) {
    int wsgroup = *groupit;
    if (wsgroup < minwsgroup && wsgroup >= 0)
      minwsgroup = wsgroup;
  }
  g_log.debug() << "[DB] Min WS Group = " << minwsgroup << "\n";

  bool from1 = getProperty("OutputWorkspaceIndexedFrom1");
  int delta_wsindex = 0;
  if (from1) {
    delta_wsindex = 1 - minwsgroup;
  }

  // Set up new workspaces
  std::set<int>::iterator groupit;
  int numoutputws = 0;
  double numnewws = static_cast<double>(m_workGroupIndexes.size());
  double wsgindex = 0.;

  for (groupit = m_workGroupIndexes.begin();
       groupit != m_workGroupIndexes.end(); ++groupit) {
    // Generate new workspace name
    bool add2output = true;
    int wsgroup = *groupit;
    std::stringstream wsname;
    if (wsgroup >= 0) {
      wsname << m_outputWSNameBase << "_" << (wsgroup + delta_wsindex);
    } else {
      wsname << m_outputWSNameBase << "_unfiltered";
      if (from1)
        add2output = false;
    }

    // Generate one of the output workspaces & Copy geometry over. But we don't
    // copy the data.
    DataObjects::EventWorkspace_sptr optws =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            API::WorkspaceFactory::Instance().create(
                "EventWorkspace", m_eventWS->getNumberHistograms(), 2, 1));
    API::WorkspaceFactory::Instance().initializeFromParent(m_eventWS, optws,
                                                           false);
    m_outputWS.insert(std::make_pair(wsgroup, optws));

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
          new API::WorkspaceProperty<DataObjects::EventWorkspace>(
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
      mProgress = 0.1 + 0.1 * wsgindex / numnewws;
      progress(mProgress, "Creating output workspace");
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
  size_t numhist = m_eventWS->getNumberHistograms();
  MatrixWorkspace_sptr corrws = boost::dynamic_pointer_cast<MatrixWorkspace>(
      WorkspaceFactory::Instance().create("Workspace2D", numhist, 2, 2));
  setProperty("OutputTOFCorrectionWorkspace", corrws);

  // Set up the size of correction and output correction workspace
  m_detTofOffsets.resize(numhist, 1.0);
  m_detTofShifts.resize(numhist, 0.0);

  // Set up detector values
  if (m_tofCorrType == CustomizedCorrect) {
    setupCustomizedTOFCorrection();
  } else if (m_tofCorrType == ElasticCorrect) {
    // Generate TOF correction from instrument's set up
    setupElasticTOFCorrection(corrws);
  } else if (m_tofCorrType == DirectCorrect) {
    // Generate TOF correction for direct inelastic instrument
    setupDirectTOFCorrection(corrws);
  } else if (m_tofCorrType == IndirectCorrect) {
    // Generate TOF correction for indirect elastic instrument
    setupIndirectTOFCorrection(corrws);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/**
  */
void FilterEvents::setupElasticTOFCorrection(API::MatrixWorkspace_sptr corrws) {
  // Get sample distance to moderator
  Geometry::Instrument_const_sptr instrument = m_eventWS->getInstrument();
  IComponent_const_sptr source =
      boost::dynamic_pointer_cast<const IComponent>(instrument->getSource());
  double l1 = instrument->getDistance(*source);

  // Get
  size_t numhist = m_eventWS->getNumberHistograms();
  for (size_t i = 0; i < numhist; ++i) {
    if (!m_vecSkip[i]) {
      IComponent_const_sptr tmpdet =
          boost::dynamic_pointer_cast<const IComponent>(
              m_eventWS->getDetector(i));
      double l2 = instrument->getDistance(*tmpdet);

      double corrfactor = (l1) / (l1 + l2);

      m_detTofOffsets[i] = corrfactor;
      corrws->dataY(i)[0] = corrfactor;
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Calculate TOF correction for direct geometry inelastic instrument
  * Time = T_pulse + TOF*0 + L1/sqrt(E*2/m)
  */
void FilterEvents::setupDirectTOFCorrection(API::MatrixWorkspace_sptr corrws) {
  // Get L1
  V3D samplepos = m_eventWS->getInstrument()->getSample()->getPos();
  V3D sourcepos = m_eventWS->getInstrument()->getSource()->getPos();
  double l1 = samplepos.distance(sourcepos);

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

  // Calculate constant (to all spectra) shift
  double constshift = l1 / sqrt(ei * 2. * PhysicalConstants::meV /
                                PhysicalConstants::NeutronMass);

  // Set up the shfit
  size_t numhist = m_eventWS->getNumberHistograms();

  g_log.debug()
      << "Calcualte direct inelastic scattering for input workspace of "
      << numhist << " spectra "
      << "storing to output workspace with " << corrws->getNumberHistograms()
      << " spectra. "
      << "\n";

  for (size_t i = 0; i < numhist; ++i) {
    m_detTofOffsets[i] = 0.0;
    m_detTofShifts[i] = constshift;

    corrws->dataY(i)[0] = 0.0;
    corrws->dataY(i)[1] = constshift;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Calculate TOF correction for indirect geometry inelastic instrument
  * Time = T_pulse + TOF - L2/sqrt(E_fix * 2 * meV / mass)
  */
void
FilterEvents::setupIndirectTOFCorrection(API::MatrixWorkspace_sptr corrws) {
  g_log.debug("Start to set up indirect TOF correction. ");

  // A constant among all spectra
  double twomev_d_mass =
      2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;
  V3D samplepos = m_eventWS->getInstrument()->getSample()->getPos();

  // Get the parameter map
  const ParameterMap &pmap = m_eventWS->constInstrumentParameters();

  // Set up the shift
  size_t numhist = m_eventWS->getNumberHistograms();

  g_log.debug() << "[DBx158] Number of histograms = " << numhist
                << ", Correction WS size = " << corrws->getNumberHistograms()
                << "\n";

  for (size_t i = 0; i < numhist; ++i) {
    if (!m_vecSkip[i]) {
      double shift;
      IDetector_const_sptr det = m_eventWS->getDetector(i);
      if (!det->isMonitor()) {
        // Get E_fix
        double efix = 0.;
        try {
          Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
          if (par) {
            efix = par->value<double>();
            g_log.debug() << "Detector: " << det->getID() << " of spectrum "
                          << i << " EFixed: " << efix << "\n";
          } else {
            g_log.warning() << "Detector: " << det->getID() << " of spectrum "
                            << i << " does not have EFixed set up."
                            << "\n";
          }
        } catch (std::runtime_error &) {
          // Throws if a DetectorGroup, use single provided value
          stringstream errmsg;
          errmsg << "Inelastic instrument detector " << det->getID()
                 << " of spectrum " << i << " does not have EFixed ";
          throw runtime_error(errmsg.str());
        }

        // Get L2
        double l2 = det->getPos().distance(samplepos);

        // Calculate shift
        shift = -1. * l2 / sqrt(efix * twomev_d_mass);

        g_log.notice() << "Detector " << i << ": "
                       << "L2 = " << l2 << ", EFix = " << efix
                       << ", Shift = " << shift << "\n";
      } else {
        // Monitor:
        g_log.warning() << "Spectrum " << i << " contains detector "
                        << det->getID() << " is a monitor. "
                        << "\n";

        shift = 0.;
      }

      // Set up the shifts
      m_detTofOffsets[i] = 1.0;
      m_detTofShifts[i] = shift;

      corrws->dataY(i)[0] = 1.0;
      corrws->dataY(i)[1] = shift;
    }
  } // ENDOF (all spectra)

  return;
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

  // Parse detector and its TOF offset (i.e., correction) to a map
  map<detid_t, double> correctmap;
  map<detid_t, double> shiftmap;
  size_t numrows = m_detCorrectWorkspace->rowCount();
  for (size_t i = 0; i < numrows; ++i) {
    TableRow row = m_detCorrectWorkspace->getRow(i);

    // Parse to map
    detid_t detid;
    double offset;
    row >> detid >> offset;
    if (offset >= 0 && offset <= 1) {
      // Valid offset (factor value)
      correctmap.insert(make_pair(detid, offset));
    } else {
      // Error, throw!
      stringstream errss;
      errss << "Correction (i.e., offset) equal to " << offset << " of row "
            << "is out of range [0, 1].";
      throw runtime_error(errss.str());
    }

    // Shift
    if (hasshift) {
      double shift;
      row >> shift;
      shiftmap.insert(make_pair(detid, shift));
    }
  } // ENDFOR(row i)

  // Check size of TOF correction map
  size_t numhist = m_eventWS->getNumberHistograms();
  if (correctmap.size() > numhist) {
    g_log.warning() << "Input correction table workspace has more detectors ("
                    << correctmap.size() << ") than input workspace "
                    << m_eventWS->name() << "'s spectra number (" << numhist
                    << ".\n";
  } else if (correctmap.size() < numhist) {
    stringstream errss;
    errss << "Input correction table workspace has more detectors ("
          << correctmap.size() << ") than input workspace " << m_eventWS->name()
          << "'s spectra number (" << numhist << ".\n";
    throw runtime_error(errss.str());
  }

  // Apply to m_detTofOffsets and m_detTofShifts
  if (usedetid) {
    // Get vector IDs
    vector<detid_t> vecDetIDs;
    vecDetIDs.resize(numhist, 0);
    // Set up the detector IDs to vecDetIDs and set up the initial value
    for (size_t i = 0; i < numhist; ++i) {
      // It is assumed that there is one detector per spectra.
      // If there are more than 1 spectrum, it is very likely to have problem
      // with correction factor
      const DataObjects::EventList events = m_eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      std::set<detid_t>::iterator detit;
      if (detids.size() != 1) {
        // Check whether there are more than 1 detector per spectra.
        stringstream errss;
        errss << "The assumption is that one spectrum has one and only one "
                 "detector. "
              << "Error is found at spectrum " << i << ".  It has "
              << detids.size() << " detectors.";
        throw runtime_error(errss.str());
      }
      detid_t detid = 0;
      for (detit = detids.begin(); detit != detids.end(); ++detit)
        detid = *detit;
      vecDetIDs[i] = detid;
    }

    // Map correction map to list
    map<detid_t, double>::iterator fiter;
    for (size_t i = 0; i < numhist; ++i) {
      detid_t detid = vecDetIDs[i];
      // correction (factor) map
      fiter = correctmap.find(detid);
      if (fiter != correctmap.end())
        m_detTofOffsets[i] = fiter->second;
      else {
        stringstream errss;
        errss << "Detector "
              << "w/ ID << " << detid << " of spectrum " << i
              << " in Eventworkspace " << m_eventWS->name()
              << " cannot be found in input TOF calibration workspace. ";
        throw runtime_error(errss.str());
      }
      // correction shift map
      fiter = shiftmap.find(detid);
      if (fiter != shiftmap.end())
        m_detTofShifts[i] = fiter->second;
    } // ENDFOR (each spectrum i)
  } else {
    // It is spectrum ID already
    map<detid_t, double>::iterator fiter;
    // correction factor
    for (fiter = correctmap.begin(); fiter != correctmap.end(); ++fiter) {
      size_t wsindex = static_cast<size_t>(fiter->first);
      if (wsindex < numhist)
        m_detTofOffsets[wsindex] = fiter->second;
      else {
        stringstream errss;
        errss << "Workspace index " << wsindex << " is out of range.";
        throw runtime_error(errss.str());
      }
    }
    // correction shift
    for (fiter = shiftmap.begin(); fiter != shiftmap.end(); ++fiter) {
      size_t wsindex = static_cast<size_t>(fiter->first);
      if (wsindex < numhist)
        m_detTofShifts[wsindex] = fiter->second;
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
          outputs.insert(std::make_pair(index, output_el));
        }
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList &input_el = m_eventWS->getEventList(iws);

      // Perform the filtering (using the splitting function and just one
      // output)
      if (mFilterByPulseTime) {
        input_el.splitByPulseTime(m_splitters, outputs);
      } else if (m_tofCorrType != NoneCorrect) {
        input_el.splitByFullTime(m_splitters, outputs, true,
                                 m_detTofOffsets[iws], m_detTofShifts[iws]);
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

  std::vector<std::string> lognames;
  this->getTimeSeriesLogNames(lognames);
  g_log.debug() << "[FilterEvents D1214]:  Number of TimeSeries Logs = "
                << lognames.size() << " to " << m_outputWS.size()
                << " outptu workspaces. \n";

  double numws = static_cast<double>(m_outputWS.size());
  double outwsindex = 0.;
  for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++wsiter) {
    int wsindex = wsiter->first;
    DataObjects::EventWorkspace_sptr opws = wsiter->second;

    // Generate a list of splitters for current output workspace
    Kernel::TimeSplitterType splitters;
    generateSplitters(wsindex, splitters);

    g_log.debug() << "[FilterEvents D1215]: Output workspace Index " << wsindex
                  << ": Name = " << opws->name()
                  << "; Number of splitters = " << splitters.size() << ".\n";

    // Skip output workspace has ZERO splitters
    if (splitters.size() == 0) {
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
          outputs.insert(std::make_pair(index, output_el));
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
      if (mFilterByPulseTime) {
        throw runtime_error(
            "It is not a good practice to split fast event by pulse time. ");
      } else if (m_tofCorrType != NoneCorrect) {
        logmessage = input_el.splitByFullTimeMatrixSplitter(
            m_vecSplitterTime, m_vecSplitterGroup, outputs, true,
            m_detTofOffsets[iws], m_detTofShifts[iws]);
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
/** Generate splitters for specified workspace index as a subset of m_splitters
 */
void FilterEvents::generateSplitters(int wsindex,
                                     Kernel::TimeSplitterType &splitters) {
  splitters.clear();
  for (size_t isp = 0; isp < m_splitters.size(); ++isp) {
    Kernel::SplittingInterval splitter = m_splitters[isp];
    int index = splitter.index();
    if (index == wsindex) {
      splitters.push_back(splitter);
    }
  }

  return;
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
    for (size_t i = 0; i < splitters.size(); ++i) {
      SplittingInterval split = splitters[i];
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
 */
void FilterEvents::getTimeSeriesLogNames(std::vector<std::string> &lognames) {
  lognames.clear();

  const std::vector<Kernel::Property *> allprop =
      m_eventWS->mutableRun().getProperties();
  for (size_t ip = 0; ip < allprop.size(); ++ip) {
    Kernel::TimeSeriesProperty<double> *timeprop =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(allprop[ip]);
    if (timeprop) {
      std::string pname = timeprop->name();
      lognames.push_back(pname);
    }
  } // FOR

  return;
}

} // namespace Mantid
} // namespace Algorithms
