/*WIKI*
This algorithm filters events from an [[EventWorkspace]] to one or multiple [[EventWorkspace]]s according to an input [[SplittersWorkspace]] containing a series of splitters (i.e., [[SplittingInterval]]s).

==== Output ====
The output will be one or multiple workspaces according to the number of index in splitters.  The output workspace name is the combination of parameter OutputWorkspaceBaseName and the index in splitter.

==== Calibration File ====
The calibration, or say correction, from the detector to sample must be consider in fast log.  Thus a calibration file is required.  The math is
 TOF_calibrated = TOF_raw * correction(detector ID).

The calibration is in column data format.

A reasonable approximation of the correction is
 correction(detector_ID) = L1/(L1+L2(detector_ID))

==== Unfiltered Events ====
Some events are not inside any splitters.  They are put to a workspace name ended with '_unfiltered'.

If input property 'OutputWorkspaceIndexedFrom1' is set to True, then this workspace shall not be outputed.

==== Difference from FilterByLogValue ====
In FilterByLogValue(), EventList.splitByTime() is used.

In FilterEvents(), if FilterByPulse is selected true, EventList.SplitByTime is called;
otherwise, EventList.SplitByFullTime() is called instead.

The difference between splitByTime and splitByFullTime is that splitByTime filters events by pulse time,
and splitByFullTime considers both pulse time and TOF.

Therefore, FilterByLogValue is not suitable for fast log filtering.

=== Comparing with other event filtering algorithms ===
Wiki page [[EventFiltering]] has a detailed introduction on event filtering in MantidPlot. 

*WIKI*/

#include "MantidAlgorithms/FilterEvents.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/LogFilter.h"

#include <sstream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FilterEvents)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FilterEvents::FilterEvents()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FilterEvents::~FilterEvents()
  {
  }
  
  void FilterEvents::initDocs()
  {    
    setWikiSummary("Filter events from an [[EventWorkspace]] to one or multiple [[EventWorkspace]]s according to a series of splitters.");
  }

  //----------------------------------------------------------------------------------------------
  /** Declare Inputs
   */
  void FilterEvents::init()
  {
    declareProperty(
          new API::WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input),
          "An input event workspace" );

    declareProperty(
          new API::WorkspaceProperty<API::Workspace>("SplitterWorkspace", "", Direction::Input),
          "An input SpilltersWorskpace for filtering");

    declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                    "The base name to use for the output workspace" );

    declareProperty(new WorkspaceProperty<TableWorkspace>("InformationWorkspace", "", Direction::Input,
                                                          PropertyMode::Optional),
                    "Optional output for the information of each splitter workspace index.");

    auto tablewsprop = new WorkspaceProperty<TableWorkspace>("DetectorTOFCorrectionWorkspace", "", Direction::Input,
                                                             PropertyMode::Optional);
    declareProperty(tablewsprop, "Name of table workspace containing the log time correction factor for each detector. ");

    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputTOFCorrectionWorkspace", "TOFCorrectWS", Direction::Output),
                    "Name of output workspace for TOF correction factor. ");

    declareProperty("FilterByPulseTime", false,
                    "Filter the event by its pulse time only for slow sample environment log.  This option can make execution of algorithm faster.  But it lowers precision.");

    declareProperty("GroupWorkspaces", false,
                    "Option to group all the output workspaces.  Group name will be OutputWorkspaceBaseName.");

    declareProperty("OutputWorkspaceIndexedFrom1", false, "If selected, the minimum output workspace is indexed from 1 and continuous. ");

    declareProperty("GenerateTOFCorrection", false, "If this option is true and user does not specify DetectorTOFCorrectionWorkspacel, "
                    "then the correction will be generated automatically by the instrument geometry. ");

    declareProperty("SplitSampleLogs", true, "If selected, all sample logs will be splitted by the  "
                    "event splitters.  It is not recommended for fast event log splitters. ");

    declareProperty("NumberOutputWS", 0, "Number of output output workspace splitted. ", Direction::Output);

    declareProperty("DBSpectrum", EMPTY_INT(), "Spectrum (workspace index) for debug purpose. ");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Execution body
   */
  void FilterEvents::exec()
  {
    // Process algorithm properties
    processProperties();

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
    if (m_toGroupWS)
    {
      mProgress = 0.9;
      progress(mProgress, "Group workspaces");

      std::string groupname = m_outputWSNameBase;
      API::IAlgorithm_sptr groupws = createChildAlgorithm("GroupWorkspaces", 0.99, 1.00, true);
      // groupws->initialize();
      groupws->setAlwaysStoreInADS(true);
      groupws->setProperty("InputWorkspaces", m_wsNames);
      groupws->setProperty("OutputWorkspace", groupname);
      groupws->execute();
      if (!groupws->isExecuted())
      {
        g_log.error() << "Grouping all output workspaces fails." << std::endl;
      }
    }

    mProgress = 1.0;
    progress(mProgress, "Completed");

    return;
  }


  void FilterEvents::processProperties()
  {
    m_eventWS = this->getProperty("InputWorkspace");
    if (!m_eventWS)
    {
      stringstream errss;
      errss << "Inputworkspace is not event workspace. ";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    // Process splitting workspace (table or data)
    API::Workspace_sptr tempws = this->getProperty("SplitterWorkspace");

    m_splittersWorkspace = boost::dynamic_pointer_cast<SplittersWorkspace>(tempws);
    if (m_splittersWorkspace)
    {
      m_useTableSplitters = true;
    }
    else
    {
      m_matrixSplitterWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tempws);
      if (m_matrixSplitterWS)
      {
        m_useTableSplitters = false;
      }
      else
      {
        throw runtime_error("Invalid type of input workspace, neither SplittersWorkspace nor MatrixWorkspace.");
      }
    }

    m_informationWS = this->getProperty("InformationWorkspace");

    m_outputWSNameBase = this->getPropertyValue("OutputWorkspaceBaseName");
    m_detCorrectWorkspace = getProperty("DetectorTOFCorrectionWorkspace");
    mFilterByPulseTime = this->getProperty("FilterByPulseTime");

    m_toGroupWS = this->getProperty("GroupWorkspaces");

    // Do correction or not?
    m_genTOFCorrection = getProperty("GenerateTOFCorrection");
    if (m_detCorrectWorkspace)
    {
      // User specify detector TOF correction, then no need to generate TOF correction
      m_doTOFCorrection = true;
      m_genTOFCorrection = false;
    }
    else if (m_genTOFCorrection)
    {
      // If no detector TOF correction workspace is specified but specified to go generate TOF
      m_doTOFCorrection = true;
    }
    else
    {
      // No correction is needed
      m_doTOFCorrection = false;
    }

    // Informatin workspace is specified?
    if (!m_informationWS)
      mWithInfo = false;
    else
      mWithInfo = true;

    m_splitSampleLogs = getProperty("SplitSampleLogs");

    // Debug spectrum
    m_dbWSIndex = getProperty("DBSpectrum");
    if (isEmpty(m_dbWSIndex))
      m_useDBSpectrum = false;
    else
      m_useDBSpectrum = true;
  }

  //----------------------------------------------------------------------------------------------
  /** Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
   *  and create a map for all workspace group number
   */
  void FilterEvents::processSplittersWorkspace()
  {
    // 1. Init data structure
    size_t numsplitters = m_splittersWorkspace->getNumberSplitters();
    m_splitters.reserve(numsplitters);

    // 2. Insert all splitters
    bool inorder = true;
    for (size_t i = 0; i < numsplitters; i ++)
    {
      m_splitters.push_back(m_splittersWorkspace->getSplitter(i));
      m_workGroupIndexes.insert(m_splitters.back().index());

      if (inorder && i > 0 && m_splitters[i] < m_splitters[i-1])
        inorder = false;
    }
    mProgress = 0.05;
    progress(mProgress);

    // 3. Order if not ordered and add workspace for events excluded
    if (!inorder)
    {
      std::sort(m_splitters.begin(), m_splitters.end());
    }

    // 4. Add extra workgroup index for unfiltered events
    m_workGroupIndexes.insert(-1);

    // 5. Add information
    if (mWithInfo)
    {
      if (m_workGroupIndexes.size() > m_informationWS->rowCount()+1)
      {
        g_log.warning() << "Input Splitters Workspace has different entries (" << m_workGroupIndexes.size() -1
                        << ") than input information workspaces (" << m_informationWS->rowCount() << "). "
                        << "  Information may not be accurate. " << std::endl;
      }
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /**
    */
  void FilterEvents::processMatrixSplitterWorkspace()
  {
    // Check input workspace validity
    const MantidVec& vecX = m_matrixSplitterWS->readX(0);
    const MantidVec& vecY = m_matrixSplitterWS->readY(0);
    size_t sizex = vecX.size();
    size_t sizey = vecY.size();
    if (sizex - sizey != 1)
      throw runtime_error("Size must be N and N-1.");

    // Assign vectors for time comparison
    m_vecSplitterTime.assign(vecX.size(), 0);
    m_vecSplitterGroup.assign(vecY.size(), -1);

    // Transform vector
    for (size_t i = 0; i < sizex; ++i)
    {
      m_vecSplitterTime[i] = static_cast<int64_t>(vecX[i]);
    }
    for (size_t i = 0; i < sizey; ++i)
    {
      m_vecSplitterGroup[i] = static_cast<int>(vecY[i]);
      m_workGroupIndexes.insert(m_vecSplitterGroup[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a list of EventWorkspace for output
   */
  void FilterEvents::createOutputWorkspaces()
  {

    // Convert information workspace to map
    std::map<int, std::string> infomap;
    if (mWithInfo)
    {
      for (size_t ir = 0; ir < m_informationWS->rowCount(); ++ ir)
      {
        API::TableRow row = m_informationWS->getRow(ir);
        int& indexws = row.Int(0);
        std::string& info = row.String(1);
        infomap.insert(std::make_pair(indexws, info));
      }
    }

    // Determine the minimum group index number
    int minwsgroup = INT_MAX;
    for (set<int>::iterator groupit = m_workGroupIndexes.begin(); groupit != m_workGroupIndexes.end(); ++groupit)
    {
      int wsgroup = *groupit;
      if (wsgroup < minwsgroup && wsgroup >= 0)
        minwsgroup = wsgroup;
    }
    g_log.debug() << "[DB] Min WS Group = " << minwsgroup << "\n";

    bool from1 = getProperty("OutputWorkspaceIndexedFrom1");
    int delta_wsindex = 0;
    if (from1)
    {
      delta_wsindex = 1-minwsgroup;
    }

    // Set up new workspaces
    std::set<int>::iterator groupit;
    int numoutputws = 0;
    double numnewws = static_cast<double>(m_workGroupIndexes.size());
    double wsgindex = 0.;

    for (groupit = m_workGroupIndexes.begin(); groupit != m_workGroupIndexes.end(); ++groupit)
    {
      // Generate new workspace name
      bool add2output = true;
      int wsgroup = *groupit;
      std::stringstream wsname;
      if (wsgroup >= 0)
      {
        wsname << m_outputWSNameBase << "_" << (wsgroup+delta_wsindex);
      }
      else
      {
        wsname << m_outputWSNameBase << "_unfiltered";
        if (from1)
          add2output = false;
      }

      // Generate one of the output workspaces & Copy geometry over. But we don't copy the data.
      DataObjects::EventWorkspace_sptr optws = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            API::WorkspaceFactory::Instance().create("EventWorkspace", m_eventWS->getNumberHistograms(), 2, 1));
      API::WorkspaceFactory::Instance().initializeFromParent(m_eventWS, optws, false);
      m_outputWS.insert(std::make_pair(wsgroup, optws));

      // Add information, including title and comment, to output workspace
      if (mWithInfo)
      {
        std::string info;
        if (wsgroup < 0)
        {
          info = "Events that are filtered out. ";
        }
        else
        {
          std::map<int, std::string>::iterator infoiter;
          infoiter = infomap.find(wsgroup);
          if (infoiter != infomap.end())
          {
            info = infoiter->second;
          }
          else
          {
            info = "This workspace has no informatin provided. ";
          }
        }
        optws->setComment(info);
        optws->setTitle(info);
      }

      // Add to output properties.  There shouldn't be any workspace (non-unfiltered) skipped from group index
      if (add2output)
      {
        // Generate output property name
        std::stringstream propertynamess;
        propertynamess << "OutputWorkspace_" << wsgroup;

        // Inserted this pair to map
        m_wsNames.push_back(wsname.str());

        // Set (property) to output workspace and set to ADS
        declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(
                          propertynamess.str(), wsname.str(), Direction::Output), "Output");
        setProperty(propertynamess.str(), optws);
        AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

        ++ numoutputws;

        g_log.debug() << "[DB9141] Created output Workspace of group = " << wsgroup << "  Property Name = "
                      << propertynamess.str() << " Workspace name = " << wsname.str()
                      << " with Number of events = " << optws->getNumberEvents() << "\n";

        // Update progress report
        mProgress = 0.1 + 0.1*wsgindex/numnewws;
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
    */
  void FilterEvents::setupDetectorTOFCalibration()
  {
    // Prepare output (class variables)
    std::vector<detid_t> vecDetIDs;
    m_detTofOffsets.clear();

    size_t numhist = m_eventWS->getNumberHistograms();
    vecDetIDs.resize(numhist, 0);
    m_detTofOffsets.resize(numhist, 1.0);

    // Create the output workspace for correction factors
    MatrixWorkspace_sptr corrws = boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", numhist, 1, 1));

    // Set up the detector IDs to vecDetIDs and set up the initial value
    for (size_t i = 0; i < numhist; ++i)
    {
      // It is assumed that there is one detector per spectra.
      // If there are more than 1 spectrum, it is very likely to have problem with correction factor
      const DataObjects::EventList events = m_eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      std::set<detid_t>::iterator detit;
      if (detids.size() != 1)
      {
        // Check whether there are more than 1 detector per spectra.
        stringstream errss;
        errss << "The assumption is that one spectrum has one and only one detector. "
              << "Error is found at spectrum " << i << ".  It has " << detids.size() << " detectors.";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
      detid_t detid = 0;
      for (detit=detids.begin(); detit!=detids.end(); ++detit)
        detid = *detit;
      vecDetIDs[i] = detid;

      corrws->dataY(i)[0] = 1.0;
      m_detTofOffsets[i] = 1.0;
    }

    // Calculate TOF correction value for all detectors
    if (m_detCorrectWorkspace)
    {
      // Obtain correction from detector calibration workspace

      // Check input workspace
      vector<string> colnames = m_detCorrectWorkspace->getColumnNames();
      if (colnames.size() < 2)
        throw runtime_error("Input table workspace is not valide.");
      else if (colnames[0].compare("DetectorID") || colnames[1].compare("Correction"))
        throw runtime_error("Input table workspace has wrong column definition.");

      // Parse detector and its TOF offset (i.e., correction) to a map
      map<detid_t, double> correctmap;
      size_t numrows = m_detCorrectWorkspace->rowCount();
      for (size_t i = 0; i < numrows; ++i)
      {
        TableRow row = m_detCorrectWorkspace->getRow(i);

        detid_t detid;
        double offset;
        row >> detid >> offset;

        correctmap.insert(make_pair(detid, offset));
      }

      // Check size of TOF correction map
      if (correctmap.size() > numhist)
      {
        g_log.warning() << "Input correction table workspace has more detectors (" << correctmap.size()
                        << ") than input workspace " << m_eventWS->name() << "'s spectra number ("
                        << numhist << ".\n";
      }
      else if (correctmap.size() < numhist)
      {
        stringstream errss;
        errss << "Input correction table workspace has more detectors (" << correctmap.size()
              << ") than input workspace " << m_eventWS->name() << "'s spectra number ("
              << numhist << ".\n";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }

      // Map correction map to list
      map<detid_t, double>::iterator fiter;
      for (size_t i = 0; i < numhist; ++i)
      {
        detid_t detid = vecDetIDs[i];
        fiter = correctmap.find(detid);
        if (fiter == correctmap.end())
        {
          stringstream errss;
          errss << "Detector " << "w/ ID << " << detid  << " of spectrum " << i << " in Eventworkspace " << m_eventWS->name()
                << " cannot be found in input TOF calibration workspace. ";
          g_log.error(errss.str());
          throw runtime_error(errss.str());
        }
        else
        {
          m_detTofOffsets[i] = fiter->second;
          corrws->dataY(i)[0] = fiter->second;
        }
      }
    }
    else if (m_genTOFCorrection)
    {
      // Generate TOF correction from instrument's set up

      // Get sample distance to moderator
      Geometry::Instrument_const_sptr instrument = m_eventWS->getInstrument();
      IComponent_const_sptr source = boost::dynamic_pointer_cast<const IComponent>(
            instrument->getSource());
      double l1 = instrument->getDistance(*source);

      // Get
      for (size_t i = 0; i < numhist; ++i)
      {
        IComponent_const_sptr tmpdet = boost::dynamic_pointer_cast<const IComponent>(m_eventWS->getDetector(i));
        double l2 = instrument->getDistance(*tmpdet);

        double corrfactor = (l1)/(l1+l2);

        m_detTofOffsets[i] = corrfactor;
        corrws->dataY(i)[0] = corrfactor;
      }
    }

    // Set output
    // Add correction workspace to output
    setProperty("OutputTOFCorrectionWorkspace", corrws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main filtering method
    * Structure: per spectrum --> per workspace
   */
  void FilterEvents::filterEventsBySplitters(double progressamount)
  {
    size_t numberOfSpectra = m_eventWS->getNumberHistograms();
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

    // Loop over the histograms (detector spectra) to do split from 1 event list to N event list
    g_log.debug() << "Number of spectra in input/source EventWorkspace = " << numberOfSpectra << ".\n";

    // FIXME Make it parallel
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws)
    {
      // FIXME Make it parallel
      // PARALLEL_START_INTERUPT_REGION

      // Get the output event lists (should be empty) to be a map
      std::map<int, DataObjects::EventList* > outputs;
      for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++ wsiter)
      {
        int index = wsiter->first;
        DataObjects::EventList* output_el = wsiter->second->getEventListPtr(iws);
        outputs.insert(std::make_pair(index, output_el));
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList& input_el = m_eventWS->getEventList(iws);

      // Perform the filtering (using the splitting function and just one output)
      if (mFilterByPulseTime)
      {
        input_el.splitByPulseTime(m_splitters, outputs);
      }
      else if (m_doTOFCorrection)
      {
        input_el.splitByFullTime(m_splitters, outputs, m_detTofOffsets[iws], m_doTOFCorrection);
      }
      else
      {
        input_el.splitByFullTime(m_splitters, outputs, 1.0, m_doTOFCorrection);
      }

      mProgress = 0.3+(progressamount-0.2)*static_cast<double>(iws)/static_cast<double>(numberOfSpectra);
      progress(mProgress, "Filtering events");

      // FIXME - Turn on parallel
      // PARALLEL_END_INTERUPT_REGION
    } // END FOR i = 0
    // PARALLEL_CHECK_INTERUPT_REGION
    // FIXME - Turn on parallel


    // Split the sample logs in each target workspace.
    progress(0.1+progressamount, "Splitting logs");

    if (!m_splitSampleLogs)
    {
      // Skip if choice is no
      g_log.notice("Sample logs are not split by user's choice.");
      return;
    }

    std::vector<std::string> lognames;
    this->getTimeSeriesLogNames(lognames);
    g_log.debug() << "[FilterEvents D1214]:  Number of TimeSeries Logs = " << lognames.size()
                  << " to " << m_outputWS.size() << " outptu workspaces. \n";

    double numws = static_cast<double>(m_outputWS.size());
    double outwsindex = 0.;
    for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++wsiter)
    {
      int wsindex = wsiter->first;
      DataObjects::EventWorkspace_sptr opws = wsiter->second;

      // Generate a list of splitters for current output workspace
      Kernel::TimeSplitterType splitters;
      generateSplitters(wsindex, splitters);

      g_log.debug() << "[FilterEvents D1215]: Output orkspace Index " << wsindex
                    << ": Name = " << opws->name() << "; Number of splitters = " << splitters.size() << ".\n";

      // Skip output workspace has ZERO splitters
      if (splitters.size() == 0)
      {
        g_log.warning() << "[FilterEvents] Workspace " << opws->name() << " Indexed @ " << wsindex <<
                           " won't have logs splitted due to zero splitter size. " << ".\n";
        continue;
      }

      // Split log
      size_t numlogs = lognames.size();
      for (size_t ilog = 0; ilog < numlogs; ++ilog)
      {
        this->splitLog(opws, lognames[ilog], splitters);
      }
      opws->mutableRun().integrateProtonCharge();

      progress(0.1+progressamount+outwsindex/numws*0.2, "Splitting logs");
      outwsindex += 1.;
    }

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Split events by splitters represented by vector
    */
  void FilterEvents::filterEventsByVectorSplitters(double progressamount)
  {
    size_t numberOfSpectra = m_eventWS->getNumberHistograms();
    // FIXME : consider to use vector to index workspace and event list
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

    // Loop over the histograms (detector spectra) to do split from 1 event list to N event list
    g_log.debug() << "Number of spectra in input/source EventWorkspace = " << numberOfSpectra << ".\n";

#if 0
    vector< map<int, EventList*> > vec_elistmap;
    for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws)
    {
      map<int, DataObjects::EventList* > outputs;
      for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++ wsiter)
      {
        int index = wsiter->first;
        DataObjects::EventList* output_el = wsiter->second->getEventListPtr(iws);
        outputs.insert(std::make_pair(index, output_el));
      }
    }
#endif

    // FIXME Make it parallel
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws)
    {
      // FIXME Make it parallel
      PARALLEL_START_INTERUPT_REGION

      // Get the output event lists (should be empty) to be a map
      map<int, DataObjects::EventList* > outputs;
      PARALLEL_CRITICAL(build_elist)
      {
        for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++ wsiter)
        {
          int index = wsiter->first;
          DataObjects::EventList* output_el = wsiter->second->getEventListPtr(iws);
          outputs.insert(std::make_pair(index, output_el));
        }
      }

      // Get a holder on input workspace's event list of this spectrum
      const DataObjects::EventList& input_el = m_eventWS->getEventList(iws);

      bool printdetail = false;
      if (m_useDBSpectrum)
          printdetail = (iws == static_cast<int64_t>(m_dbWSIndex));

      // Perform the filtering (using the splitting function and just one output)
      std::string logmessage("");
      if (mFilterByPulseTime)
      {
        throw runtime_error("It is not a good practice to split fast event by pulse time. ");
        // input_el.splitByPulseTimeMatrixSplitter(m_vecSplitterTime, m_vecSplitterGroup, outputs);
      }
      else if (m_doTOFCorrection)
      {
        logmessage = input_el.splitByFullTimeMatrixSplitter(m_vecSplitterTime, m_vecSplitterGroup, outputs,
                                               m_detTofOffsets[iws], m_doTOFCorrection, printdetail);
      }
      else
      {
        logmessage = input_el.splitByFullTimeMatrixSplitter(m_vecSplitterTime, m_vecSplitterGroup, outputs, 1.0,
                                                            m_doTOFCorrection,
                                                            printdetail);
      }

      mProgress = 0.3+(progressamount-0.2)*static_cast<double>(iws)/static_cast<double>(numberOfSpectra);
      progress(mProgress, "Filtering events");

      if (printdetail)
        g_log.notice(logmessage);

      // FIXME - Turn on parallel
      PARALLEL_END_INTERUPT_REGION
    } // END FOR i = 0
    PARALLEL_CHECK_INTERUPT_REGION
    // FIXME - Turn on parallel


    // Finish (1) adding events and splitting the sample logs in each target workspace.
    progress(0.1+progressamount, "Splitting logs");

#if 0
    std::vector<std::string> lognames;
    this->getTimeSeriesLogNames(lognames);
    g_log.debug() << "[FilterEvents D1214]:  Number of TimeSeries Logs = " << lognames.size()
                  << " to " << m_outputWS.size() << " outptu workspaces. \n";

    double numws = static_cast<double>(m_outputWS.size());
    double outwsindex = 0.;
    for (wsiter = m_outputWS.begin(); wsiter != m_outputWS.end(); ++wsiter)
    {
      int wsindex = wsiter->first;
      DataObjects::EventWorkspace_sptr opws = wsiter->second;

      // Generate a list of splitters for current output workspace
      Kernel::TimeSplitterType splitters;
      generateSplitters(wsindex, splitters);

      g_log.debug() << "[FilterEvents D1215]: Output orkspace Index " << wsindex
                    << ": Name = " << opws->name() << "; Number of splitters = " << splitters.size() << ".\n";

      // Skip output workspace has ZERO splitters
      if (splitters.size() == 0)
      {
        g_log.warning() << "[FilterEvents] Workspace " << opws->name() << " Indexed @ " << wsindex <<
                           " won't have logs splitted due to zero splitter size. " << ".\n";
        continue;
      }

      // Split log
      size_t numlogs = lognames.size();
      for (size_t ilog = 0; ilog < numlogs; ++ilog)
      {
        this->splitLog(opws, lognames[ilog], splitters);
      }
      opws->mutableRun().integrateProtonCharge();

      progress(0.1+progressamount+outwsindex/numws*0.2, "Splitting logs");
      outwsindex += 1.;
    }
#else
    g_log.notice("Splitters in format of Matrixworkspace are not recommended to split sample logs. ");
#endif

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Generate splitters for specified workspace index as a subset of m_splitters
   */
  void FilterEvents::generateSplitters(int wsindex, Kernel::TimeSplitterType& splitters)
  {
    splitters.clear();
    for (size_t isp = 0; isp < m_splitters.size(); ++ isp)
    {
      Kernel::SplittingInterval splitter = m_splitters[isp];
      int index = splitter.index();
      if (index == wsindex)
      {
        splitters.push_back(splitter);
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Split a log by splitters
   */
  void FilterEvents::splitLog(EventWorkspace_sptr eventws, std::string logname, TimeSplitterType& splitters)
  {
    Kernel::TimeSeriesProperty<double>* prop =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(eventws->mutableRun().getProperty(logname));
    if (!prop)
    {
      g_log.warning() << "Log " << logname << " is not TimeSeriesProperty.  Unable to split." << std::endl;
      return;
    }
    else
    {
      for (size_t i = 0; i < splitters.size(); ++i)
      {
        SplittingInterval split = splitters[i];
        g_log.debug() << "[FilterEvents DB1226] Going to filter workspace " << eventws->name() << ": "
                      << "log name = " << logname << ", duration = " << split.duration()
                      << " from " << split.start() << " to " << split.stop() << ".\n";
      }
    }

    prop->filterByTimes(splitters);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Get all filterable logs' names
   */
  void FilterEvents::getTimeSeriesLogNames(std::vector<std::string>& lognames)
  {
    lognames.clear();

    const std::vector<Kernel::Property*> allprop = m_eventWS->mutableRun().getProperties();
    for (size_t ip = 0; ip < allprop.size(); ++ip)
    {
      Kernel::TimeSeriesProperty<double>* timeprop = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(allprop[ip]);
      if (timeprop)
      {
        std::string pname = timeprop->name();
        lognames.push_back(pname);
      }
    } // FOR

    return;
  }

} // namespace Mantid
} // namespace Algorithms
