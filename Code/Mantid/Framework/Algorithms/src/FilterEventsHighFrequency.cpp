/*WIKI*

Filter events for VULCAN

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAlgorithms/FilterEventsHighFrequency.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <algorithm>
#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(FilterEventsHighFrequency)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FilterEventsHighFrequency::FilterEventsHighFrequency()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FilterEventsHighFrequency::~FilterEventsHighFrequency()
  {
  }

  void FilterEventsHighFrequency::initDocs(){

    return;
  }

  /*
   * Declare input/output properties
   */
  void FilterEventsHighFrequency::init(){

    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputEventWorkspace", "", Direction::InOut),
        "Input EventWorkspace.  Each spectrum corresponds to 1 pixel");
    bool optional = true;
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("SampleEnvironmentWorkspace", "", Direction::Input, optional),
        "Input 2D workspace storing sample environment data along with absolute time");
    this->declareProperty("LogName", "", "Log's name to filter events.");
    this->declareProperty(new API::FileProperty("InputCalFile", "", API::FileProperty::Load, ".dat"),
        "Input pixel TOF calibration file in column data format");
    this->declareProperty("SensorToSampleOffset", 0.0, "Offset in micro-second from sample to sample environment sensor");
    this->declareProperty("ValueLowerBoundary", 0.0, "Lower boundary of sample environment value for selected events");
    this->declareProperty("ValueUpperBoundary", 0.0, "Upper boundary of sample environment value for selected events");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("OutputWorkspace", "Anonymous", Direction::Output),
        "Output EventWorkspace.");
    this->declareProperty("T0", 0.0, "Earliest time of the events to be selected.  It is a relative time to starting time in second.");
    this->declareProperty("Tf", 0.0, "Latest time of the events to be selected.  It is a relative time to starting time in second.");
    this->declareProperty(new API::FileProperty("OutputDirectory", "", API::FileProperty::OptionalDirectory),
        "Directory of all output files");

    return;
  }

  /*
   * Main body to execute the algorithm
   */
  void FilterEventsHighFrequency::exec(){

    // 1. Get property
    eventWS = this->getProperty("InputEventWorkspace");
    const std::string outputdir = this->getProperty("OutputDirectory");
    seWS = this->getProperty("SampleEnvironmentWorkspace");
    const std::string calfilename = this->getProperty("InputCalFile");
    double tempoffset = this->getProperty("SensorToSampleOffset");
    mSensorSampleOffset = static_cast<int64_t>(tempoffset*1000);
    mLowerLimit = this->getProperty("ValueLowerBoundary");
    mUpperLimit = this->getProperty("ValueUpperBoundary");

    std::string logname = this->getProperty("LogName");

    // b) Some time issues
    const API::Run& runlog = eventWS->run();
    std::string runstartstr = runlog.getProperty("run_start")->value();
    Kernel::DateAndTime runstart(runstartstr);
    mRunStartTime = runstart;
    double t0 = this->getProperty("T0");
    double tf = this->getProperty("Tf");
    if (tf <= t0){
      g_log.error() << "User defined filter starting time (T0 = " << t0 << ") is later than ending time (Tf = " << tf << ")" << std::endl;
      throw std::invalid_argument("User input T0 and Tf error!");
    }
    mFilterT0 = runstart + t0;
    mFilterTf = runstart + tf;

    g_log.debug() << "T0 = " << mFilterT0 << ";  Tf = " << mFilterTf << std::endl;

    // 2. Check and process input
    // a) Event Workspace
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
      const DataObjects::EventList events = eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      if (detids.size() != 1){
        g_log.error() << "Spectrum " << i << " has more than 1 detectors (" << detids.size() << "). Algorithm does not support! " << std::endl;
        throw std::invalid_argument("EventWorkspace error");
      }
    }

    // b) Sample environment workspace:  increment workspace?  If log file is given, then read from log file and ignore the workspace
    if (logname.size() > 0){
      this->processTimeLog(logname);
      g_log.notice() << "Using input EventWorkspace's log " << logname << std::endl;
    } else {
      this->processTimeLog(seWS);
      g_log.notice() << "Using input Workspace2D " << seWS->name() << " as log" << std::endl;
    }

    /*  Skipped as all events will be filtered by log!
    for (size_t i = 1; i < seWS->dataX(0).size(); i++){
      if (seWS->dataX(0)[i] <= seWS->dataX(0)[i-1]){
        g_log.error() << "Sample E. Workspace: data " << i << " = " << seWS->dataX(0)[i] << " < data " << i-1 << " = " << seWS->dataX(0)[i-1] << std::endl;
        g_log.error() << "dT = " << (seWS->dataX(0)[i]-seWS->dataX(0)[i-1]) << std::endl;
        throw std::invalid_argument("Input Sample E. Workspace is not in ascending order!");
      }
    }
    */

    // 3. Read calibration file
    importCalibrationFile(calfilename);

    // 4. Build new Workspace
    createEventWorkspace();

    // 5. Filter
    filterEvents();

    g_log.debug() << "Trying to set Output Workspace: " << outputWS->getName() << std::endl;
    this->setProperty("OutputWorkspace", outputWS);
    g_log.debug() << "Output Workspace is set!" << " Number of Events in Spectrum 0 = " << outputWS->getEventList(0).getNumberEvents() << std::endl;

    return;

    // -2. Study
    /*
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
      DataObjects::EventList events = eventWS->getEventList(i);
    }

    // -1: Check
    size_t numput = 2000;
    if (seWS->dataX(0).size() < numput)
      numput = seWS->dataX(0).size();
    g_log.notice() << "Output First " << numput << " Entries in S.E. Log  (CHECK THE CODE!)" << std::endl;


    std::string opfname = outputdir+"/"+"pulse.dat";
    std::ofstream ofs;
    ofs.open(opfname.c_str(), std::ios::out);
    for (size_t i = 0; i < numput; i ++){
      ofs << static_cast<int64_t>(seWS->dataX(0)[i]) << "  " << seWS->dataY(0)[i] << std::endl;
    }
    ofs.close();

    return;
    */
  } // exec

  /*
   * Convert input workspace to vectors for fast access
   */
  void FilterEventsHighFrequency::processTimeLog(DataObjects::Workspace2D_const_sptr ws2d)
  {
    g_log.debug() << "Not Implemented Yet Work Input Workspace2D " << ws2d->getName() << std::endl;

    return;
  }

  /*
   * Convert time log to vectors for fast access
   */
  void FilterEventsHighFrequency::processTimeLog(std::string logname){

    const API::Run& runlogs = eventWS->run();
    Kernel::TimeSeriesProperty<double> * fastfreqlog
        = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData(logname) );

    std::vector<Kernel::DateAndTime> timevec = fastfreqlog->timesAsVector();
    for (size_t i = 0; i < timevec.size(); i ++){
      mSETimes.push_back(timevec[i].total_nanoseconds());
      double tv = fastfreqlog->getSingleValue(timevec[i]);
      mSEValues.push_back(tv);
      /*
      if (i < 20){
        g_log.notice() << "VZ Test  Log Vector " << i << " : " << timevec[i] << ", " << tv << std::endl;
      }
       */
    }

    // CHECK
    // TODO   Remove this section
    g_log.debug() << "VZ Test  Total Log lookup table size = " << mSEValues.size() << ", " << timevec.size() << std::endl;
    for (size_t i = 1; i < timevec.size(); i ++){
      if (timevec[i-1] >= timevec[i]){
        g_log.error() << "Time [" << i << "] = " << timevec[i] << "  is earlier than Time@" << (i-1) << std::endl;
      }
    }
    /**********************/

    return;
  }

  /*
   * Import TOF calibration/offset file for each pixel.
   */
  void FilterEventsHighFrequency::importCalibrationFile(std::string calfilename){

    detid_t indet;
    int64_t offset; //
    double doffset; // Assuming the file gives offset in micro-second

    // 1. Check workspace
    if (!eventWS){
      g_log.error() << "Required to import EventWorkspace before calling importCalibrationFile()" << std::endl;
      throw std::invalid_argument("Calling function in wrong order!");
    }

    // 2. Open file
    std::ifstream ifs;
    mCalibDetectorIDs.clear();
    mCalibOffsets.clear();

    try{
      // a. Successful scenario
      ifs.open(calfilename.c_str(), std::ios::in);

      for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
        // i. each pixel:  get detector ID from EventWorkspace
        const DataObjects::EventList events = this->eventWS->getEventList(i);
        std::set<detid_t> detids = events.getDetectorIDs();
        std::set<detid_t>::iterator detit;
        detid_t detid = 0;
        for (detit=detids.begin(); detit!=detids.end(); ++detit)
          detid = *detit;

        // ii. read file
        ifs >> indet >> doffset;
        offset = static_cast<int64_t>(doffset*1000);

        // iii. store
        if (indet != detid){
          g_log.error() << "Error!  Line " << i << " Should read in pixel " << detid << "  but read in " << indet << std::endl;
        }

        mCalibDetectorIDs.push_back(detid);
        mCalibOffsets.push_back(offset);
      }

      ifs.close();

    } catch (std::ifstream::failure & e){
      // b. Using faking offset/calibration
      g_log.error() << "Open calibration/offset file " << calfilename << " error " << std::endl;
      g_log.notice() << "Using fake detector offset/calibration" << std::endl;

      // Reset vectors
      mCalibDetectorIDs.clear();
      mCalibOffsets.clear();

      for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
        const DataObjects::EventList events = this->eventWS->getEventList(i);
        std::set<detid_t> detids = events.getDetectorIDs();
        std::set<detid_t>::iterator detit;
        detid_t detid = 0;
        for (detit=detids.begin(); detit!=detids.end(); ++detit)
          detid = *detit;

        mCalibDetectorIDs.push_back(detid);
        mCalibOffsets.push_back(0);
      }

    } // try-catch

    return;
  }

  /*
   * Create an output EventWorkspace w/o any events
   */
  void FilterEventsHighFrequency::createEventWorkspace(){

    // 1. Initialize:use dummy numbers for arguments, for event workspace it doesn't matter
    outputWS = DataObjects::EventWorkspace_sptr(new DataObjects::EventWorkspace());
    outputWS->setName("FilteredWorkspace");
    outputWS->initialize(1,1,1);

    // 2. Set the units
    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    outputWS->setYUnit("Counts");
    // TODO: Give a meaningful title later
    outputWS->setTitle("Filtered");

    // 3. Add the run_start property:
    //    TODO:  Figure out how to get mutable run's property out!
    // Kernel::Property* runstarttime = eventWS->mutableRun().getProperty("run_start");
    // outputWS->mutableRun().addProperty(eventWS->mutableRun().getProperty("run_start"), true);

    // Kernel::Property* runnumber = eventWS->mutableRun().getProperty("run_number");
    // outputWS->mutableRun().addProperty("run_number", runnumber);

    // 4. Instrument
    IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");
    // Now execute the sub-algorithm. Catch and log any error, but don't stop.
    loadInst->setPropertyValue("InstrumentName", eventWS->getInstrument()->getName());
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", outputWS);
    loadInst->setProperty("RewriteSpectraMap", true);
    loadInst->executeAsSubAlg();
    // Populate the instrument parameters in this workspace - this works around a bug
    outputWS->populateInstrumentParameters();

    // 5. ??? Is pixel mapping file essential???

    // 6. Build spectrum and event list
    // a) We want to pad out empty pixels.
    detid2det_map detector_map;
    outputWS->getInstrument()->getDetectors(detector_map);

    g_log.debug() << "VZ: 6a) detector map size = " << detector_map.size() << std::endl;

    // b) determine maximum pixel id
    detid2det_map::iterator it;
    detid_t detid_max = 0; // seems like a safe lower bound
    for (it = detector_map.begin(); it != detector_map.end(); it++)
      if (it->first > detid_max)
        detid_max = it->first;

    // c) Pad all the pixels and Set to zero
    std::vector<std::size_t> pixel_to_wkspindex;
    pixel_to_wkspindex.reserve(detid_max+1); //starting at zero up to and including detid_max
    pixel_to_wkspindex.assign(detid_max+1, 0);
    size_t workspaceIndex = 0;
    for (it = detector_map.begin(); it != detector_map.end(); it++)
    {
      if (!it->second->isMonitor())
      {
        pixel_to_wkspindex[it->first] = workspaceIndex;
        DataObjects::EventList & spec = outputWS->getOrAddEventList(workspaceIndex);
        spec.addDetectorID(it->first);
        // Start the spectrum number at 1
        spec.setSpectrumNo(specid_t(workspaceIndex+1));
        workspaceIndex += 1;
      }
    }
    outputWS->doneAddingEventLists();

    // Clear
    pixel_to_wkspindex.clear();

    g_log.debug() << "VZ (End of createEventWorkspace): Total spectrum number = " << outputWS->getNumberHistograms() << std::endl;

    return;

  }

  /*
   * Filter events from eventWS to outputWS
   */
  void FilterEventsHighFrequency::filterEvents(){

    double shortest_tof = 1.0E10;
    double longest_tof = -1;

    // 1. Sort the workspace (event) in the order absolute time
    API::IAlgorithm_sptr sort1 = createSubAlgorithm("SortEvents");
    sort1->initialize();
    sort1->setProperty("InputWorkspace", eventWS);
    sort1->setProperty("SortBy", "Pulse Time + TOF");
    sort1->execute();

    g_log.notice() << "Sorting input EventWS is done!" << std::endl;
    g_log.debug() << "Calibration Offset Size = " << mCalibOffsets.size() << std::endl;

    // 2. Filter by each spectrum
    for (size_t ip=0; ip<eventWS->getNumberHistograms(); ip++){
      // For each spectrum
      // a. Offset
      int64_t timeoffset = mSensorSampleOffset+mCalibOffsets[ip];

      // b. Get all events
      DataObjects::EventList events = eventWS->getEventList(ip);
      std::vector<int64_t>::iterator abstimeit;
      std::vector<DataObjects::TofEvent> newevents;
      // g_log.notice() << "VZ (DelLater): Spectrum " << ip << " # (Events) = " << events.getNumberEvents() << std::endl;

      // c. Filter the event
      // TODO  Add filter for T0, Tf
      size_t posoffsetL = 0;
      size_t posoffsetU = 0;
      size_t indexL = 0;
      size_t indexU = events.getNumberEvents()-1;
      bool islow = true;

      for (size_t iv=0; iv<events.getNumberEvents(); iv++){

        // 0. Determine index
        size_t index;

        if (islow){
          index = indexL;
          indexL ++;
        } else {
          index = indexU;
          indexU --;
        }

        // i.  get raw event & time
        DataObjects::TofEvent rawevent = events.getEvent(index);
        int64_t mtime = rawevent.m_pulsetime.total_nanoseconds()+static_cast<int64_t>(rawevent.m_tof*1000)+timeoffset;

        // ii. filter out if time falls out of (T0, Tf)
        if (mtime < mFilterT0.total_nanoseconds() || mtime > mFilterTf.total_nanoseconds()){
          islow = !islow;
          continue;
          /*
          if (ip == 0 && iv < 10)
            g_log.notice() << "VZSpecial:  Outside boundary" << std::endl;
          */
        }

        // iii. search... need to consider more situation as outside of boundary, on the grid and etc
        abstimeit = std::lower_bound(mSETimes.begin()+posoffsetL, mSETimes.end()-posoffsetU, mtime);
        size_t mindex;
        if (*abstimeit == mtime){
          // (1) On the grid
          mindex = size_t(abstimeit-mSETimes.begin());
        }
        else if (abstimeit == mSETimes.begin()){
          // (2) On first grid or out of lower bound
          mindex = size_t(abstimeit-mSETimes.begin());
        } else {
          mindex = size_t(abstimeit-mSETimes.begin())-1;
        }

        // TODO  Delete this test section after test done
        if (mindex >= mSETimes.size()){
          size_t numsetimes = mSETimes.size();
          int64_t dt = mtime - mRunStartTime.total_nanoseconds();
          g_log.error() << "Locate " << mtime << "  Time 0 = " << mSETimes[0] << ", Time f = " << mSETimes[numsetimes-1] << std::endl;
          g_log.error() << "Time = " << mtime << "  T-T0  = " << (static_cast<double>(dt)*1.0E-9) << " sec" << std::endl;
          throw std::invalid_argument("Flag 1616:  Wrong in searching!!!");
        }
        // TODO  Delete this test section after test done
        if ((mtime >= mSETimes[0] && mtime < mSETimes[mSETimes.size()-1]) && (mtime < mSETimes[mindex] || mtime >= mSETimes[mindex+1]) ){
          size_t numsetimes = mSETimes.size();
          g_log.error() << "Low offset = " << posoffsetL << ",  Up offset = " << posoffsetU << std::endl;
          g_log.error() << "Locate " << mtime << "  Time 0 = " << mSETimes[0] << ", Time f = " << mSETimes[numsetimes-1] << std::endl;
          g_log.error() << "Locate " << mtime << " @ " << mindex << " [" << mSETimes[mindex] << ", " << mSETimes[mindex+1] << "] " << std::endl;
          g_log.error() << "Iterator value = " << *abstimeit << std::endl;
          throw std::invalid_argument("Flag 1623!  Found but not within the range");
        }

        // iv. update position offset
        if (islow){
          // Update lower side offset
          posoffsetL = mindex;
        } else {
          // Update upper side offset
          if (mindex < events.getNumberEvents()){
            posoffsetU = events.getNumberEvents()-mindex-1;
          } else {
            posoffsetU = 0;
          }
        }

        islow = !islow;

        // v. filter in/out?
        double msevalue = mSEValues[mindex];
        if (msevalue >= mLowerLimit && msevalue <= mUpperLimit){
          DataObjects::TofEvent newevent(rawevent.m_tof, rawevent.m_pulsetime);
          newevents.push_back(newevent);
        }
      } // ENDFOR: each event

      // 3. Add to outputWS
      /*
      if (ip < 10){
        g_log.notice() << "VZSpecial: Spec " << ip << " # New Events = " << newevents.size() << std::endl;
      }
      */

      DataObjects::EventList* neweventlist = outputWS->getEventListPtr(ip);
      for (size_t iv=0; iv<newevents.size(); iv++){
        neweventlist->addEventQuickly(newevents[iv]);
        if (newevents[iv].m_tof > longest_tof){
          longest_tof = newevents[iv].m_tof;
        } else if (newevents[iv].m_tof < shortest_tof){
          shortest_tof = newevents[iv].m_tof;
        }
      }
    } // ENDFOR: each spectrum

    // 4. Add a dummy histogramming
    //    create a default X-vector for histogramming, with just 2 bins.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(2);
    xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
    xRef[1] = longest_tof + 1;
    outputWS->setAllX(axis);

    g_log.debug() << "End of filterEvents()" << std::endl;

    return;
  }


} // namespace Mantid
} // namespace Algorithms

























