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

using namespace Mantid::Kernel;
using namespace Mantid::API;

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

  }

  /*
   * Declare Inputs
   */
  void FilterEvents::init()
  {
    declareProperty(
      new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputWorkspace","",Direction::Input),
      "An input event workspace" );

    declareProperty("OutputWorkspaceBaseName", "OutputWorkspace",
      "The base name to use for the output workspace" );

    declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("SplittersInformationWorkspace", "", Direction::Input, PropertyMode::Optional),
        "Optional output for the information of each splitter workspace index.");

    declareProperty(
        new API::WorkspaceProperty<DataObjects::SplittersWorkspace>("InputSplittersWorkspace", "", Direction::Input),
        "An input SpilltersWorskpace for filtering");

    this->declareProperty(new API::FileProperty("DetectorCalibrationFile", "", API::FileProperty::OptionalLoad, ".dat"),
        "Input pixel TOF calibration file in column data format");

    this->declareProperty("FilterByPulseTime", false,
        "Filter the event by its pulse time only for slow sample environment log.  This option can make execution of algorithm faster.  But it lowers precision.");

    this->declareProperty("GroupWorkspaces", false,
        "Option to group all the output workspaces.  Group name will be OutputWorkspaceBaseName.");

    return;
  }

  /*
   * Execution body
   */
  void FilterEvents::exec()
  {
    // 1. Get inputs
    mEventWorkspace = this->getProperty("InputWorkspace");
    mSplittersWorkspace = this->getProperty("InputSplittersWorkspace");
    std::string outputwsnamebase = this->getProperty("OutputWorkspaceBaseName");
    std::string detcalfilename = this->getProperty("DetectorCalibrationFile");
    mFilterByPulseTime = this->getProperty("FilterByPulseTime");

    mInformationWS = this->getProperty("SplittersInformationWorkspace");
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

    progress(0.2);
    importDetectorTOFCalibration(detcalfilename);

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
      API::IAlgorithm_sptr groupws = createSubAlgorithm("GroupWorkspaces", 0.99, 1.00, true);
      // groupws->initialize();
      groupws->setAlwaysStoreInADS(true);
      groupws->setProperty("InputWorkspaces", mWsNames);
      groupws->setProperty("OutputWorkspace", groupname);
      groupws->execute();
      if (!groupws->isExecuted())
      {
        g_log.error() << "Grouping all output workspaces fails." << std::endl;
      }
    }

    return;
  }

  /*
   * Convert SplitterWorkspace object to TimeSplitterType (sorted vector)
   * and create a map for all workspace group number
   */
  void FilterEvents::processSplittersWorkspace()
  {
    // 1. Init data structure
    size_t numsplitters = mSplittersWorkspace->getNumberSplitters();
    mSplitters.reserve(numsplitters);

    // 2. Insert all splitters
    bool inorder = true;
    for (size_t i = 0; i < numsplitters; i ++)
    {
      mSplitters.push_back(mSplittersWorkspace->getSplitter(i));
      mWorkspaceGroups.insert(mSplitters.back().index());

      if (inorder && i > 0 && mSplitters[i] < mSplitters[i-1])
        inorder = false;
    }
    mProgress = 0.5;
    progress(mProgress);

    // 3. Order if not ordered and add workspace for events excluded
    if (!inorder)
    {
      std::sort(mSplitters.begin(), mSplitters.end());
    }
    mWorkspaceGroups.insert(-1);

    // 4. Add information
    if (mWithInfo)
    {
      if (mWorkspaceGroups.size() > mInformationWS->rowCount()+1)
      {
        g_log.warning() << "Input Splitters Workspace has different entries (" << mWorkspaceGroups.size() -1 <<
            ") than input information workspaces (" << mInformationWS->rowCount() << "). "
            << "  Information may not be accurate. " << std::endl;
      }
    }

    return;
  }

  /*
   * Create a list of EventWorkspace for output
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

    // Set up new workspaces
    std::set<int>::iterator groupit;
    for (groupit = mWorkspaceGroups.begin(); groupit != mWorkspaceGroups.end(); ++groupit)
    {
      // 1. Get workspace name
      int wsgroup = *groupit;
      std::stringstream wsname;
      wsname << outputwsnamebase << "_" << wsgroup;
      std::stringstream parname;
      parname << "OutputWorkspace_" << wsgroup;

      // 2. Generate one of the output workspaces & Copy geometry over. But we don't copy the data.
      DataObjects::EventWorkspace_sptr optws = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          API::WorkspaceFactory::Instance().create("EventWorkspace", mEventWorkspace->getNumberHistograms(), 2, 1));
      API::WorkspaceFactory::Instance().initializeFromParent(mEventWorkspace, optws, false);

      //    Add information
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

      // 3. Set to map
      mOutputWorkspaces.insert(std::make_pair(wsgroup, optws));

      // 4. Set to output workspace
      this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(parname.str(), wsname.str(), Direction::Output), "Output");
      this->setProperty(parname.str(), optws);
      mWsNames.push_back(wsname.str());
      AnalysisDataService::Instance().addOrReplace(wsname.str(), optws);

      g_log.debug() << "DB9141  Output Workspace:  Group = " << wsgroup << "  Property Name = " << parname.str() <<
          " Workspace name = " << wsname.str() <<
          " with Number of events = " << optws->getNumberEvents() << std::endl;
    } // ENDFOR


    return;
  }

  /*
   * Import the detector calibration on TOF
   */
  void FilterEvents::importDetectorTOFCalibration(std::string detcalfilename)
  {
    detid_t indet;

    // 1. Check workspace
    if (!mEventWorkspace)
    {
      g_log.error() << "Required to import EventWorkspace before calling importCalibrationFile()" << std::endl;
      throw std::invalid_argument("Calling function in wrong order!");
    }

    // 2. Prepare output
    mCalibDetectorIDs.clear();
    mCalibOffsets.clear();
    size_t numhist = mEventWorkspace->getNumberHistograms();
    mCalibDetectorIDs.reserve(numhist);
    mCalibOffsets.reserve(numhist);

    // 3. Read file?
    bool readcalfile = true;
    if (detcalfilename.empty())
    {
      readcalfile = false;
    }

    if (readcalfile)
    {
      try{
        // a. Open file
        std::ifstream ifs;
        ifs.open(detcalfilename.c_str(), std::ios::in);

        double doffset;
        for (size_t i = 0; i < numhist; i ++)
        {
          // i. each pixel:  get detector ID from EventWorkspace
          const DataObjects::EventList events = mEventWorkspace->getEventList(i);
          std::set<detid_t> detids = events.getDetectorIDs();
          std::set<detid_t>::iterator detit;
          detid_t detid = 0;
          for (detit=detids.begin(); detit!=detids.end(); ++detit)
            detid = *detit;

          // ii. read file
          ifs >> indet >> doffset;

          // iii. store
          if (indet != detid){
            g_log.error() << "Calibration File Error!  Line " << i << " should read in pixel " << detid << "  but read in " << indet
                << "\nAbort to reading calibration file!"<< std::endl;
            readcalfile = false;
            break;
          }
          else if (doffset < 0 || doffset > 1.0)
          {
            g_log.error() << "Calibration File Error!  Line " << i << " have ratio offset outside (0,1) " << detid << "  but read in " << indet
                <<"\nAbort to reading calibration file!"<< std::endl;
            readcalfile = false;
            break;
          }
          else
          {
            mCalibDetectorIDs.push_back(detid);
            mCalibOffsets.push_back(doffset);
          }
        }
        ifs.close();
      }
      catch (std::ifstream::failure&)
      {
        g_log.error() << "Calibration File Error!  Open calibration/offset file " << detcalfilename << " error " << std::endl;
        mCalibDetectorIDs.clear();
        mCalibOffsets.clear();
        readcalfile = false;
      }
    } // If-readcalfile

    // 4. Use default/dummy offset calibration = 1.0
    if (!readcalfile)
    {
      g_log.notice() << "Using default detector offset/calibration" << std::endl;

      for (size_t i = 0; i < mEventWorkspace->getNumberHistograms(); i ++)
      {
        std::set<detid_t> detids = mEventWorkspace->getEventList(i).getDetectorIDs();
        std::set<detid_t>::iterator detit;
        detid_t detid = 0;
        for (detit=detids.begin(); detit!=detids.end(); ++detit)
          detid = *detit;

        mCalibDetectorIDs.push_back(detid);
        mCalibOffsets.push_back(1.0);
      }
    } // If NOT Read-calibration-file

    return;
  }

  /*
   * Main filtering method
   */
  void FilterEvents::filterEventsBySplitters()
  {
    size_t numberOfSpectra = mEventWorkspace->getNumberHistograms();
    std::map<int, DataObjects::EventWorkspace_sptr>::iterator wsiter;

    // 1. Loop over the histograms (detector spectra)
    // FIXME Make it parallel
    // PARALLEL_FOR_NO_WSP_CHECK()
    for (int64_t iws = 0; iws < int64_t(numberOfSpectra); ++iws)
    {
      // PARALLEL_START_INTERUPT_REGION

      // a) Get the output event lists (should be empty) to be a map
      std::map<int, DataObjects::EventList* > outputs;
      for (wsiter = mOutputWorkspaces.begin(); wsiter != mOutputWorkspaces.end(); ++ wsiter)
      {
        int index = wsiter->first;
        DataObjects::EventList* output_el = wsiter->second->getEventListPtr(iws);
        outputs.insert(std::make_pair(index, output_el));
      }

      // b) and this is the input event list
      const DataObjects::EventList& input_el = mEventWorkspace->getEventList(iws);

      // c) Perform the filtering (using the splitting function and just one output)
      input_el.splitByFullTime(mSplitters, outputs, mCalibOffsets[iws]);

      mProgress = 0.2+0.8*double(iws)/double(numberOfSpectra);
      progress(mProgress);

      // PARALLEL_END_INTERUPT_REGION
    } // END FOR i = 0
    // PARALLEL_CHECK_INTERUPT_REGION

    // 2. Finish adding events and To split/filter the runs for each workspace
    std::vector<std::string> lognames;
    this->getTimeSeriesLogNames(lognames);
    g_log.notice() << "DB1019:  Number of TimeSeries Logs = " << lognames.size() << std::endl;

    for (wsiter = mOutputWorkspaces.begin(); wsiter != mOutputWorkspaces.end(); ++wsiter)
    {
      int wsindex = wsiter->first;
      DataObjects::EventWorkspace_sptr opws = wsiter->second;

      // 2a Done adding event
      opws->doneAddingEventLists();

      // 2b To split/filter the selected run of the workspace output
      Kernel::TimeSplitterType splitters;
      generateSplitters(wsindex, splitters);

      g_log.notice() << "Workspace Index " << wsindex << "  Number of Splitters = " << splitters.size() << std::endl;

      if (splitters.size() == 0)
      {
        g_log.notice() << "Workspace " << opws->name() << " Indexed @ " << wsindex <<
            " won't have logs splitted due to zero splitter size. " << std::endl;
      }

      for (size_t ilog = 0; ilog < lognames.size(); ++ilog)
      {
        this->splitLog(opws, lognames[ilog], splitters);
      }
    }

    return;
  }


  /*
   * Generate splitters for specified workspace index
   */
  void FilterEvents::generateSplitters(int wsindex, Kernel::TimeSplitterType& splitters)
  {
    splitters.clear();
    for (size_t isp = 0; isp < mSplitters.size(); ++ isp)
    {
      Kernel::SplittingInterval splitter = mSplitters[isp];
      int index = splitter.index();
      if (index == wsindex)
      {
        splitters.push_back(splitter);
      }
    }

    return;
  }

  /*
   * Split a log by splitters
   */
  void FilterEvents::splitLog(DataObjects::EventWorkspace_sptr eventws, std::string logname, Kernel::TimeSplitterType& splitters)
  {

    bool print = true;

    Kernel::TimeSeriesProperty<double>* prop =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(eventws->mutableRun().getProperty(logname));
    if (!prop)
    {
      g_log.warning() << "Log " << logname << " is not TimeSeriesProperty.  Unable to split." << std::endl;
      return;
    }

    // 1. Split to many splitters
    Kernel::TimeSeriesProperty<double>* splitprop = new Kernel::TimeSeriesProperty<double>(logname);
    prop->splitByTime(splitters, splitprop);

    g_log.notice() << "DB1056 Log Name = " << logname << "  Splitter Size = " << splitters.size()
        << "  Splitted Property Size = " << splitprop->size() << std::endl;

    // 2. Replace
    eventws->mutableRun().addProperty(splitprop, true);

    return;
  }


  /*
   * Get all filterable logs' names
   */
  void FilterEvents::getTimeSeriesLogNames(std::vector<std::string>& lognames)
  {
    lognames.clear();

    const std::vector<Kernel::Property*> allprop = mEventWorkspace->mutableRun().getProperties();
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
