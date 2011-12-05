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

#include "fstream"

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

    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("InputEventWorkspace", "", Direction::Input),
        "Input EventWorkspace.  Each spectrum corresponds to 1 pixel");
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("SampleEnvironmentWorkspace", "", Direction::Input),
        "Input 2D workspace storing sample environment data along with absolute time");
    this->declareProperty(new API::FileProperty("InputCalFile", "", API::FileProperty::Load, "event.dat"),
        "Input pixel TOF calibration file in column data format");

    this->declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output EventWorkspace.");
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

    // 2. Check
    // a) Event Workspace
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
      const DataObjects::EventList events = eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      if (detids.size() != 1){
        g_log.error() << "Spectrum " << i << " has more than 1 detectors (" << detids.size() << "). Algorithm does not support! " << std::endl;
        throw std::invalid_argument("EventWorkspace error");
      }
    }

    // b) Sample environment workspace:  increment workspace?
    for (size_t i = 1; i < seWS->dataX(0).size(); i++){
      if (seWS->dataX(0)[i] <= seWS->dataX(0)[i-1]){
        g_log.error() << "Sample E. Workspace: data " << i << " = " << seWS->dataX(0)[i] << " < data " << i-1 << " = " << seWS->dataX(0)[i-1] << std::endl;
        g_log.error() << "dT = " << (seWS->dataX(0)[i]-seWS->dataX(0)[i-1]) << std::endl;
        break;
        // throw std::invalid_argument("Input Sample E. Workspace is not in ascending order!");
      }
    }

    // 3. Read calibration file
    importCalibrationFile(calfilename);

    // 4. Build new Workspace
    createEventWorkspace();
    this->setProperty("OutputWorkspace", outputWS);

    // 5. Filter
    filterEvents();

    // -2. Study
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
      DataObjects::EventList events = eventWS->getEventList(i);
    }

    // -1: Check
    std::string opfname = outputdir+"/"+"pulse.dat";
    std::ofstream ofs;
    ofs.open(opfname.c_str(), std::ios::out);
    for (size_t i = 0; i < 2000; i ++){
      ofs << static_cast<int64_t>(seWS->dataX(0)[i]) << "  " << seWS->dataY(0)[i] << std::endl;
    }
    ofs.close();

    return;
  }

  void FilterEventsHighFrequency::importCalibrationFile(std::string calfilename){

    if (!eventWS){
      g_log.error() << "Required to import EventWorkspace before calling importCalibrationFile()" << std::endl;
      throw std::invalid_argument("Calling function in wrong order!");
    }

    g_log.notice() << "VZ:  Fake at this time!" << std::endl;
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++){
      const DataObjects::EventList events = this->eventWS->getEventList(i);
      std::set<detid_t> detids = events.getDetectorIDs();
      std::set<detid_t>::iterator detit;
      detid_t detid = 0;
      for (detit=detids.begin(); detit!=detids.end(); ++detit)
        detid = *detit;

      mCalibDetectorIDs.push_back(detid);
      mCalibOffsets.push_back(0.0);
    }

    return;
  }

  /*
   * Create an output EventWorkspace w/o any events
   */
  void FilterEventsHighFrequency::createEventWorkspace(){

    // 1. Initialize:use dummy numbers for arguments, for event workspace it doesn't matter
    outputWS = DataObjects::EventWorkspace_sptr(new DataObjects::EventWorkspace());
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

    g_log.notice() << "VZ: 6a) detector map size = " << detector_map.size() << std::endl;

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

    g_log.notice() << "VZ: Total spectrum number = " << outputWS->getNumberHistograms() << std::endl;

    return;

  }

  /*
   * Filter events from eventWS to outputWS
   */
  void FilterEventsHighFrequency::filterEvents(){

    // TODO  Fake case first to prototype how to add events to an EventWorkspace
    g_log.error() << "filterEvents is a fake implementation now!" << std::endl;

    double shortest_tof = 1.0E10;
    double longest_tof = -1;

    for (size_t ip=0; ip<eventWS->getNumberHistograms(); ip++){
      // For each spectrum
      // 1. Get all events
      DataObjects::EventList events = eventWS->getEventList(ip);

      std::vector<DataObjects::TofEvent> newevents;

      // 2. Filter the event
      //    TODO  Fake here
      for (size_t iv=0; iv<events.getNumberEvents(); iv++){
        DataObjects::TofEvent rawevent = events.getEvent(iv);
        DataObjects::TofEvent newevent(rawevent.m_tof, rawevent.m_pulsetime);
        newevents.push_back(newevent);
      }

      // 3. Add to outputWS
      DataObjects::EventList* neweventlist = outputWS->getEventListPtr(ip);
      for (size_t iv=0; iv<newevents.size(); iv++){
        neweventlist->addEventQuickly(newevents[iv]);
        if (newevents[iv].m_tof > longest_tof){
          longest_tof = newevents[iv].m_tof;
        } else if (newevents[iv].m_tof < shortest_tof){
          shortest_tof = newevents[iv].m_tof;
        }
      }
    } // ENDFOR

    // 4. Add a dummy histogramming
    //    create a default X-vector for histogramming, with just 2 bins.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(2);
    xRef[0] = shortest_tof - 1; //Just to make sure the bins hold it all
    xRef[1] = longest_tof + 1;
    outputWS->setAllX(axis);

    return;

  }


} // namespace Mantid
} // namespace Algorithms

























