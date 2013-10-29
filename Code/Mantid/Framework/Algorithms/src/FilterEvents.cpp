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
          new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input),
          "An input event workspace" );

    declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
                    "The base name to use for the output workspace" );

    declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("InformationWorkspace", "", Direction::Input, PropertyMode::Optional),
        "Optional output for the information of each splitter workspace index.");

    declareProperty(
        new API::WorkspaceProperty<DataObjects::SplittersWorkspace>("SplitterWorkspace", "", Direction::Input),
        "An input SpilltersWorskpace for filtering");

    auto tablewsprop = new WorkspaceProperty<TableWorkspace>("DetectorTOFCorrectionWorkspace", "", Direction::Input, PropertyMode::Optional);
    declareProperty(tablewsprop, "Name of table workspace containing the log time correction factor for each detector. ");

    this->declareProperty("FilterByPulseTime", false,
        "Filter the event by its pulse time only for slow sample environment log.  This option can make execution of algorithm faster.  But it lowers precision.");

    this->declareProperty("GroupWorkspaces", false,
        "Option to group all the output workspaces.  Group name will be OutputWorkspaceBaseName.");

    declareProperty("OutputWorkspaceIndexedFrom1", false, "If selected, the minimum output workspace is indexed from 1 and continuous. ");

    declareProperty("NumberOutputWorkspace", 0, "Number of output output workspace splitted. ", Direction::Output);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Execution body
   */
  void FilterEvents::exec()
  {
    // 1. Get inputs
    m_eventWS = this->getProperty("InputWorkspace");
    if (!m_eventWS)
    {
      stringstream errss;
      errss << "Inputworkspace is not event workspace. ";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    mSplittersWorkspace = this->getProperty("SplitterWorkspace");
    mInformationWS = this->getProperty("InformationWorkspace");

    std::string outputwsnamebase = this->getProperty("OutputWorkspaceBaseName");
    m_detCorrectWorkspace = getProperty("DetectorTOFCorrectionWorkspace");
    mFilterByPulseTime = this->getProperty("FilterByPulseTime");

    if (!mInformationWS)
    {
      mWithInfo = false;
    }
    else
    {
      mWithInfo = true;
    }

    // 2. Process inputs
    mProgress = 0.0;
    progress(mProgress, "Processing SplittersWorkspace.");
    processSplittersWorkspace();

    mProgress = 0.1;
    progress(mProgress, "Create Output Workspaces.");
    createOutputWorkspaces(outputwsnamebase);

    /*
    DateAndTime splitter_t0 = m_splitters[0].start();
    DateAndTime splitter_tf = m_splitters.back().stop();
    Kernel::TimeSeriesProperty<double>* protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(mEventWorkspace->run().getProperty("proton_charge"));
    DateAndTime startime = mEventWorkspace->getFirstPulseTime();
    DateAndTime endtime = protonchargelog->lastTime();

    int64_t diff_start = splitter_t0.totalNanoseconds()-startime.totalNanoseconds();
    int64_t diff_end = endtime.totalNanoseconds() - splitter_tf.totalNanoseconds();

    stringstream dbinfo;
    dbinfo << "1st splitter starts " << diff_start << " (ns) from first pulse time. "
           << "Last splitter ends " << diff_end << " (ns) before last proton charge log time.";
    g_log.notice(dbinfo.str());
    */

    progress(0.2);
    importDetectorTOFCalibration();

    // 3. Filter Events
    mProgress = 0.20;
    progress(mProgress, "Filter Events.");
    filterEventsBySplitters();

    mProgress = 1.0;
    progress(mProgress);

    // 4. Optional to group detector
    bool togroupws = this->getProperty("GroupWorkspaces");
    if (togroupws)
    {
      std::string groupname = outputwsnamebase;
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

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
   *  and create a map for all workspace group number
   */
  void FilterEvents::processSplittersWorkspace()
  {
    // 1. Init data structure
    size_t numsplitters = mSplittersWorkspace->getNumberSplitters();
    m_splitters.reserve(numsplitters);

    // 2. Insert all splitters
    bool inorder = true;
    for (size_t i = 0; i < numsplitters; i ++)
    {
      m_splitters.push_back(mSplittersWorkspace->getSplitter(i));
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
      if (m_workGroupIndexes.size() > mInformationWS->rowCount()+1)
      {
        g_log.warning() << "Input Splitters Workspace has different entries (" << m_workGroupIndexes.size() -1
                        << ") than input information workspaces (" << mInformationWS->rowCount() << "). "
                        << "  Information may not be accurate. " << std::endl;
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a list of EventWorkspace for output
   */
  void FilterEvents::createOutputWorkspaces(std::string outputwsnamebase)
  {
    // Convert information workspace to map
    std::map<int, std::string> infomap;
    if (mWithInfo)
    {
      for (size_t ir = 0; ir < mInformationWS->rowCount(); ++ ir)
      {
        API::TableRow row = mInformationWS->getRow(ir);
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
    for (groupit = m_workGroupIndexes.begin(); groupit != m_workGroupIndexes.end(); ++groupit)
    {
      // Generate new workspace name
      bool add2output = true;
      int wsgroup = *groupit;
      std::stringstream wsname;
      if (wsgroup >= 0)
        wsname << outputwsnamebase << "_" << (wsgroup+delta_wsindex);
      else
      {
        wsname << outputwsnamebase << "_unfiltered";
        if (from1)
          add2output = false;
      }

      // Generate one of the output workspaces & Copy geometry over. But we don't copy the data.
      DataObjects::EventWorkspace_sptr optws = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", m_eventWS->getNumberHistograms(), 2, 1));
      API::WorkspaceFactory::Instance().initializeFromParent(m_eventWS, optws, false);

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
        std::stringstream propertyname;
        propertyname << "OutputWorkspace_" << wsgroup;

        // Inserted this pair to map
        m_outputWS.insert(std::make_pair(wsgroup, optws));
        m_wsNames.push_back(wsname.str());

        // Set (property) to output workspace and set to ADS
        declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(
                          propertyname.str(), wsname.str(), Direction::Output), "Output");
        setProperty(propertyname.str(), optws);
        AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

        ++ numoutputws;

        g_log.debug() << "DB9141  Output Workspace:  Group = " << wsgroup << "  Property Name = " << propertyname.str() <<
                         " Workspace name = " << wsname.str() <<
                         " with Number of events = " << optws->getNumberEvents() << std::endl;
      }
    } // ENDFOR

    setProperty("NumberOutputWorkspace", numoutputws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Parse TOF-correction table workspace to vectors
   */
  void FilterEvents::importDetectorTOFCalibration()
  {
    // 1. Prepare output
    m_detectorIDs.clear();
    m_detTofOffsets.clear();

    size_t numhist = m_eventWS->getNumberHistograms();
    m_detectorIDs.resize(numhist, 0);
    m_detTofOffsets.resize(numhist, 1.0);

    // 2. Set the detector IDs
    for (size_t i = 0; i < numhist; ++i)
    {
      // FIXME - current implementation assumes one detector per spectra
      const DataObjects::EventList events = m_eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      std::set<detid_t>::iterator detit;
      if (detids.size() != 1)
      {
        stringstream errss;
        errss << "The assumption is that one spectrum has one and only one detector. "
              << "Error is found at spectrum " << i << ".  It has " << detids.size() << " detectors.";
        g_log.error(errss.str());
        throw runtime_error(errss.str());
      }
      detid_t detid = 0;
      for (detit=detids.begin(); detit!=detids.end(); ++detit)
        detid = *detit;
      m_detectorIDs[i] = detid;
    }

    // 3. Apply the correction table if it applies
    if (m_detCorrectWorkspace)
    {
      // If a detector calibration workspace is present

      // a) Check input workspace
      vector<string> colnames = m_detCorrectWorkspace->getColumnNames();
      if (colnames.size() < 2)
        throw runtime_error("Input table workspace is not valide.");
      else if (colnames[0].compare("DetectorID") || colnames[1].compare("Correction"))
        throw runtime_error("Input table workspace has wrong column definition.");

      // b) Parse detector to a map
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

      // c) Map correction map to list
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

      map<detid_t, double>::iterator fiter;
      for (size_t i = 0; i < numhist; ++i)
      {
        detid_t detid = m_detectorIDs[i];
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
        }
      }
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main filtering method
    * Structure: per spectrum --> per workspace
   */
  void FilterEvents::filterEventsBySplitters()
  {
    size_t numberOfSpectra = m_eventWS->getNumberHistograms();
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

    // Loop over the histograms (detector spectra) to do split from 1 event list to N event list
    g_log.information() << "[FilterEvents F1206] Number of spectra = " << numberOfSpectra << ".\n";

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
      input_el.splitByFullTime(m_splitters, outputs, m_detTofOffsets[iws]);

      mProgress = 0.2+0.8*double(iws)/double(numberOfSpectra);
      progress(mProgress);

      // FIXME - Turn on parallel
      // PARALLEL_END_INTERUPT_REGION
    } // END FOR i = 0
    // FIXME - Turn on parallel
    // PARALLEL_CHECK_INTERUPT_REGION

    // Finish (1) adding events and splitting the sample logs in each target workspace.
    std::vector<std::string> lognames;
    this->getTimeSeriesLogNames(lognames);
    g_log.debug() << "[FilterEvents D1214]:  Number of TimeSeries Logs = " << lognames.size()
                  << " to " << m_outputWS.size() << " outptu workspaces. \n";

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
    }

    return;
  }


  /*
   * Generate splitters for specified workspace index
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
  void FilterEvents::splitLog(DataObjects::EventWorkspace_sptr eventws, std::string logname, Kernel::TimeSplitterType& splitters)
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
