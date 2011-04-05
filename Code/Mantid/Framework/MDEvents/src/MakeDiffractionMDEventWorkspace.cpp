#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ProgressText.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/MakeDiffractionMDEventWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Algorithms;

namespace Mantid
{
namespace MDEvents
{

  bool DODEBUG = false;

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(MakeDiffractionMDEventWorkspace)
  
  /// Sets documentation strings for this algorithm
  void MakeDiffractionMDEventWorkspace::initDocs()
  {
    this->setWikiSummary("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace. If the OutputWorkspace exists, then events are added to it.");
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MakeDiffractionMDEventWorkspace::MakeDiffractionMDEventWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MakeDiffractionMDEventWorkspace::~MakeDiffractionMDEventWorkspace()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void MakeDiffractionMDEventWorkspace::init()
  {
    //TODO: Make sure in units are okay
    declareProperty(new WorkspaceProperty<EventWorkspace>("InputWorkspace","",Direction::Input), "An input EventWorkspace.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");
    declareProperty(new PropertyWithValue<bool>("ClearInputWorkspace", false, Direction::Input), "Clear the events from the input workspace during conversion, to save memory.");
  }


  /// Our MDEvent dimension
  typedef MDEvent<3> MDE;


  //----------------------------------------------------------------------------------------------
  /** Convert an event list to 3D q-space and add it to the MDEventWorkspace
   *
   * @tparam T :: the type of event in the input EventList (TofEvent, WeightedEvent, etc.)
   * @param workspaceIndex :: index into the workspace
   */
  template <class T>
  void MakeDiffractionMDEventWorkspace::convertEventList(int workspaceIndex)
  {
    EventList & el = in_ws->getEventList(workspaceIndex);
    size_t numEvents = el.getNumberEvents();

    // Get the position of the detector there.
    std::set<int>& detectors = el.getDetectorIDs();
    if (detectors.size() > 0)
    {
      // The 3D MDEvents that will be added into the MDEventWorkspce
      std::vector<MDE> out_events;
      out_events.reserve( el.getNumberEvents() );

      // Warn if the event list is the sum of more than one detector ID
      if (detectors.size() != 1)
      {
        g_log.warning() << "Event list at workspace index " << workspaceIndex << " has " << detectors.size() << " detectors. Only 1 detector ID per pixel is supported.\n";
        return;
      }

      int detID = *(detectors.begin());
      IDetector_sptr det = allDetectors[detID];

      // Vector between the sample and the detector
      V3D detPos = det->getPos() - samplePos;

      // Neutron's total travelled distance
      double distance = detPos.norm() + l1;

      // Detector direction normalized to 1
      V3D detDir = detPos / detPos.norm();

      // The direction of momentum transfer = the output beam direction - input beam direction (normalized)
      V3D Q_dir = detDir - beamDir;
      double Q_dir_x = Q_dir.X();
      double Q_dir_y = Q_dir.Y();
      double Q_dir_z = Q_dir.Z();

      //TODO: Rotate into sample frame (take out goniometer rotation)
      //TODO: Convert to HKL, on option.


      //std::cout << wi << " : " << el.getNumberEvents() << " events. Pos is " << detPos << std::endl;

      // This little dance makes the getting vector of events more general (since you can't overload by return type).
      typename std::vector<T> * events_ptr;
      getEventsFrom(el, events_ptr);
      typename std::vector<T> & events = *events_ptr;

      // Iterators to start/end
      typename std::vector<T>::iterator it = events.begin();
      typename std::vector<T>::iterator it_end = events.end();
      for (; it != it_end; it++)
      {
        // Time of flight of neutron in seconds
        double tof = it->tof() * 1e-6;
        // Wavenumber = momentum/h_bar = mass*distance/time / h_bar
        double wavenumber = (PhysicalConstants::NeutronMass * distance) / (tof * PhysicalConstants::h_bar);
        // Convert to units of Angstroms^-1
        wavenumber *= 1e-10;

        // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
        CoordType center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};
        //std::cout << center[0] << "," << center[1] << "," << center[2] << "\n";

        // Build a MDEvent
        out_events.push_back( MDE(it->weight(), it->errorSquared(), center) );
      }

      // Clear out the EventList to save memory
      if (ClearInputWorkspace)
      {

        // Track how much memory you cleared
        size_t memoryCleared = el.getMemorySize();
        // Clear it now
        el.clear();
        // For Linux with tcmalloc, make sure memory goes back, if you've cleared 200 Megs
        MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, 2e8);
      }

      // Add them to the MDEW
      ws->addEvents(out_events);
    }
    prog->reportIncrement(numEvents, "Adding Events");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void MakeDiffractionMDEventWorkspace::exec()
  {
    Timer tim, timtotal;
    CPUTimer cputim, cputimtotal;

    // Input workspace
    in_ws = getProperty("InputWorkspace");
    if (!in_ws)
      throw std::invalid_argument("No input event workspace was passed to algorithm.");

    ClearInputWorkspace = getProperty("ClearInputWorkspace");

    // Try to get the output workspace
    IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    ws = boost::dynamic_pointer_cast<MDEventWorkspace3>( i_out );

    if (!ws)
    {
      // Create an output workspace with 3 dimensions.
      size_t nd = 3;
      i_out = MDEventFactory::CreateMDEventWorkspace(nd, "MDEvent");
      ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(i_out);

      // Give all the dimensions
      std::string names[3] = {"Qx", "Qy", "Qz"};
      for (size_t d=0; d<nd; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(names[d], names[d], "Angstroms^-1", -100.0, +100.0, 1);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      BoxController_sptr newbc(new BoxController(3));
      newbc->setSplitInto(10);
      newbc->setSplitThreshold(1000);
      newbc->setMaxDepth(8);
      ws->setBoxController(newbc);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
    }

    if (!ws)
      throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

    BoxController_sptr bc = ws->getBoxController();
    if (!bc)
      throw std::runtime_error("Output MDEventWorkspace does not have a BoxController!");

    // Extract some parameters global to the instrument
    AlignDetectors::getInstrumentParameters(in_ws->getInstrument(),l1,beamline,beamline_norm, samplePos);
    beamline_norm = beamline.norm();
    beamDir = beamline / beamline.norm();

    //To get all the detector ID's
    in_ws->getInstrument()->getDetectors(allDetectors);

    size_t totalCost = in_ws->getNumberEvents();
    prog = new Progress(this, 0, 1.0, totalCost);
//    if (DODEBUG) prog = new ProgressText(0, 1.0, totalCost, true);
//    if (DODEBUG) prog->setNotifyStep(1);

    // Create the thread pool that will run all of these.
    ThreadScheduler * ts = new ThreadSchedulerLargestCost();
    ThreadPool tp(ts);

    // To track when to split up boxes
    size_t eventsAdded = 0;
    size_t lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
    if (DODEBUG) std::cout << cputim << ": initial setup. There are " << lastNumBoxes << " MDBoxes.\n";

    for (int wi=0; wi < in_ws->getNumberHistograms(); wi++)
    {
      // Equivalent of: this->convertEventList(wi);
      EventList & el = in_ws->getEventList(wi);

      // We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
      boost::function<void ()> func;
      switch (el.getEventType())
      {
      case TOF:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<TofEvent>, &*this, wi);
        break;
      case WEIGHTED:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<WeightedEvent>, &*this, wi);
        break;
      case WEIGHTED_NOTIME:
        func = boost::bind(&MakeDiffractionMDEventWorkspace::convertEventList<WeightedEventNoTime>, &*this, wi);
        break;
      default:
        throw std::runtime_error("EventList had an unexpected data type!");
      }

      // Give this task to the scheduler
      double cost = el.getNumberEvents();
      ts->push( new FunctionTask( func, cost) );

      // Keep a running total of how many events we've added
      eventsAdded += cost;
      if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes))
      {
        if (DODEBUG) std::cout << cputim << ": Added tasks worth " << eventsAdded << " events.\n";
        // Do all the adding tasks
        tp.joinAll();
        if (DODEBUG) std::cout << cputim << ": Performing the addition of these events.\n";

        // Now do all the splitting tasks
        ws->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          prog->doReport("Splitting Boxes");
        tp.joinAll();

        // Count the new # of boxes.
        lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
        if (DODEBUG) std::cout << cputim << ": Performing the splitting. There are now " << lastNumBoxes << " boxes.\n";
        eventsAdded = 0;
      }
    }

    if (DODEBUG) std::cout << cputim << ": We've added tasks worth " << eventsAdded << " events.\n";

    tp.joinAll();
    if (DODEBUG) std::cout << cputim << ": Performing the FINAL addition of these events.\n";

    // Do a final splitting of everything
    ws->splitAllIfNeeded(ts);
    tp.joinAll();
    if (DODEBUG) std::cout << cputim << ": Performing the FINAL splitting of boxes. There are now " << ws->getBoxController()->getTotalNumMDBoxes() <<" boxes\n";


    // Recount totals at the end.
    cputim.reset();
#ifndef MDEVENTS_MDGRIDBOX_ONGOING_SIGNAL_CACHE
    ws->refreshCache();
#endif
    if (DODEBUG) std::cout << cputim << ": Performing the refreshCache().\n";

    if (DODEBUG)
    {
      std::cout << "Workspace has " << ws->getNPoints() << " events. This took " << cputimtotal << " in total.\n";
      std::vector<std::string> stats = ws->getBoxControllerStats();
      for (size_t i=0; i<stats.size(); ++i)
        std::cout << stats[i] << "\n";
      std::cout << std::endl;
    }


    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));
  }



} // namespace Mantid
} // namespace MDEvents

