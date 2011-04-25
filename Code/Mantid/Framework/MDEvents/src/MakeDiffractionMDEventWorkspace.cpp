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

  bool DODEBUG = true;


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
  
  //  template<typename MDE, size_t nd>
//  bool pointContained(double * coords)
//
//  template<typename MDE, size_t nd>
//  void evaluateBox(MDBox<MDE, nd> & box, bool fullyContained)
//  {
//    if (fullyContained)
//    {
//      // MDBox is fully contained - use the cached value.
//      //signal += box.getSignal();
//    }
//    else
//    {
//      // Go through each event
//    }
//  }



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

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("Q (sample frame)");
    propOptions.push_back("HKL");
    declareProperty("OutputDimensions", "Q (lab frame)",new ListValidator(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the neutron in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the neutron in the frame of the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices."
       );

    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
        "Correct the weights of events with by multiplying by the Lorentz formula: sin(theta)^2 / lambda^4");

    declareProperty(new PropertyWithValue<bool>("BinarySplit", false, Direction::Input),
        "Should the MDEventWorkspace use binary splitting (use grid splitting otherwise).");
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
      V3D Q_dir_lab_frame = detDir - beamDir;

      // Multiply by the rotation matrix to convert to Q in the sample frame (take out goniometer rotation)
      // (or to HKL, if that's what the matrix is)
      V3D Q_dir = mat * Q_dir_lab_frame;

      // For speed we extract the components.
      double Q_dir_x = Q_dir.X();
      double Q_dir_y = Q_dir.Y();
      double Q_dir_z = Q_dir.Z();

      // For lorentz correction, calculate  sin(theta))^2
      double sin_theta_squared = 0;
      if (LorentzCorrection)
      {
        // Scattering angle = angle between neutron beam direction and the detector (scattering) direction
        double theta = detDir.angle(beamDir);
        sin_theta_squared = sin(theta);
        sin_theta_squared = sin_theta_squared * sin_theta_squared; // square it
      }

      /** Constant that you divide by tof (in usec) to get wavenumber in ang^-1 :
       * Wavenumber (in ang^-1) =  (PhysicalConstants::NeutronMass * distance) / ((tof (in usec) * 1e-6) * PhysicalConstants::h_bar) * 1e-10; */
      const double wavenumber_in_angstrom_times_tof_in_microsec =
          (PhysicalConstants::NeutronMass * distance * 1e-10) / (1e-6 * PhysicalConstants::h_bar);

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
        // Get the wavenumber in ang^-1 using the previously calculated constant.
        double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / it->tof();

        // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
        CoordType center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};

        if (LorentzCorrection)
        {
          //double lambda = 1.0/wavenumber;
          // (sin(theta))^2 / wavelength^4
          float correct = float( sin_theta_squared * wavenumber*wavenumber*wavenumber*wavenumber * sin_theta_squared );
          // Push the MDEvent but correct the weight.
          out_events.push_back( MDE(float(it->weight()*correct), float(it->errorSquared()*correct*correct), center) );
        }
        else
        {
          // Push the MDEvent with the same weight
          out_events.push_back( MDE(float(it->weight()), float(it->errorSquared()), center) );
        }
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

    // ---------------------- Extract properties --------------------------------------
    ClearInputWorkspace = getProperty("ClearInputWorkspace");
    std::string OutputDimensions = getPropertyValue("OutputDimensions");
    LorentzCorrection = getProperty("LorentzCorrection");

    // Input workspace
    in_ws = getProperty("InputWorkspace");
    if (!in_ws)
      throw std::invalid_argument("No input event workspace was passed to algorithm.");

    // Try to get the output workspace
    IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    ws = boost::dynamic_pointer_cast<MDEventWorkspace3>( i_out );

    // Initalize the matrix to 3x3 identity
    mat = Geometry::Matrix<double>(3,3);
    mat.identityMatrix();

    // ----------------- Handle the type of output -------------------------------------

    std::string dimensionNames[3] = {"Qx", "Qy", "Qz"};
    std::string dimensionUnits = "Angstroms^-1";
    if (OutputDimensions == "Q (sample frame)")
    {
      // TODO: Set the matrix based on goniometer angles
    }
    else if (OutputDimensions == "HKL")
    {
      // TODO: Set the matrix based on UB etc.
      dimensionNames[0] = "H";
      dimensionNames[1] = "K";
      dimensionNames[2] = "L";
      dimensionUnits = "lattice";
    }

    if (ws)
    {
      // Check that existing workspace dimensions make sense with the desired one (using the name)
      if (ws->getDimension(0)->getName() != dimensionNames[0])
        throw std::runtime_error("The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.");
    }



    // ------------------- Create the output workspace if needed ------------------------
    if (!ws)
    {
      // Create an output workspace with 3 dimensions.
      size_t nd = 3;
      i_out = MDEventFactory::CreateMDEventWorkspace(nd, "MDEvent");
      ws = boost::dynamic_pointer_cast<MDEventWorkspace3>(i_out);

      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits, -50.0, +50.0, 1);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      BoxController_sptr newbc(new BoxController(3));
      bool BinarySplit = this->getProperty("BinarySplit");
      if (BinarySplit)
      {
        newbc->setBinarySplit(true);
        newbc->setSplitThreshold(20);
        newbc->setMaxDepth(30);
      }
      else
      {
        newbc->setBinarySplit(false);
        newbc->setSplitInto(5);
        newbc->setSplitThreshold(1500);
        newbc->setMaxDepth(20);
      }
      ws->setBoxController(newbc);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
    }

    ws->splitBox();


    if (!ws)
      throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

    BoxController_sptr bc = ws->getBoxController();
    if (!bc)
      throw std::runtime_error("Output MDEventWorkspace does not have a BoxController!");


    // ------------------- Cache values that are common for all ---------------------------
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
      double cost = double(el.getNumberEvents());
      ts->push( new FunctionTask( func, cost) );

      // Keep a running total of how many events we've added
      eventsAdded += el.getNumberEvents();
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

