/*WIKI* 



The algorithm takes every event in a [[EventWorkspace]] from detector/time-of-flight space, and converts it into reciprocal space, and places the resulting MDEvents into a [[MDEventWorkspace]].

The conversion can be done either to Q-space in the lab or sample frame, or to HKL of the crystal.

If the OutputWorkspace does NOT already exist, a default one is created. In order to define more precisely the parameters of the [[MDEventWorkspace]], use the [[CreateMDWorkspace]] algorithm first.



*WIKI*/
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ProgressText.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid
{
namespace MDEvents
{

  bool DODEBUG = true;


  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(ConvertToDiffractionMDWorkspace)
  
  /// Sets documentation strings for this algorithm
  void ConvertToDiffractionMDWorkspace::initDocs()
  {
    this->setWikiSummary("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with events in reciprocal space (Qx, Qy, Qz) from an input EventWorkspace. If the OutputWorkspace exists, then events are added to it.");
  }

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  ConvertToDiffractionMDWorkspace::ConvertToDiffractionMDWorkspace()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  ConvertToDiffractionMDWorkspace::~ConvertToDiffractionMDWorkspace()
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void ConvertToDiffractionMDWorkspace::init()
  {
    //TODO: Make sure in units are okay
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
        "An input Workspace. If you specify a Workspace2D, it gets converted to "
        "an EventWorkspace using ConvertToEventWorkspace.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");
    declareProperty(new PropertyWithValue<bool>("Append", false, Direction::Input),
        "Append events to the output workspace. The workspace is replaced if unchecked.");
    declareProperty(new PropertyWithValue<bool>("ClearInputWorkspace", false, Direction::Input),
        "Clear the events from the input workspace during conversion, to save memory.");

    std::vector<std::string> propOptions;
    propOptions.push_back("Q (lab frame)");
    propOptions.push_back("Q (sample frame)");
    propOptions.push_back("HKL");
    declareProperty("OutputDimensions", "Q (lab frame)",new ListValidator(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices."
       );

    declareProperty(new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
        "Correct the weights of events with by multiplying by the Lorentz formula: sin(theta)^2 / lambda^4");

    // Box controller properties. These are the defaults
    this->initBoxControllerProps("5" /*SplitInto*/, 1500 /*SplitThreshold*/, 20 /*MaxRecursionDepth*/);

    std::vector<double> extents(2,0);
    extents[0]=-50;extents[1]=+50;
    declareProperty(
      new ArrayProperty<double>("Extents", extents),
      "A comma separated list of min, max for each dimension,\n"
      "specifying the extents of each dimension. Optional, default +-50 in each dimension.");

  }


  /// Our MDLeanEvent dimension
  typedef MDLeanEvent<3> MDE;


  //----------------------------------------------------------------------------------------------
  /** Convert an event list to 3D q-space and add it to the MDEventWorkspace
   *
   * @tparam T :: the type of event in the input EventList (TofEvent, WeightedEvent, etc.)
   * @param workspaceIndex :: index into the workspace
   */
  template <class T>
  void ConvertToDiffractionMDWorkspace::convertEventList(int workspaceIndex)
  {
    EventList & el = in_ws->getEventList(workspaceIndex);
    size_t numEvents = el.getNumberEvents();

    // Get the position of the detector there.
    std::set<detid_t>& detectors = el.getDetectorIDs();
    if (detectors.size() > 0)
    {
      // The 3D MDEvents that will be added into the MDEventWorkspce
      std::vector<MDE> out_events;
      out_events.reserve( el.getNumberEvents() );

      // Get the detector (might be a detectorGroup for multiple detectors)
      // or might return an exception if the detector is not in the instrument definition
      IDetector_const_sptr det;
      try
      {
        det = in_ws->getDetector(workspaceIndex);
      }
      catch (Exception::NotFoundError &)
      {
        this->failedDetectorLookupCount++;
        return;
      }

      // Vector between the sample and the detector
      V3D detPos = det->getPos() - samplePos;

      // Neutron's total travelled distance
      double distance = detPos.norm() + l1;

      // Detector direction normalized to 1
      V3D detDir = detPos / detPos.norm();

      // The direction of momentum transfer in the inelastic convention ki-kf
      //  = input beam direction (normalized to 1) - output beam direction (normalized to 1)
      V3D Q_dir_lab_frame = beamDir - detDir;

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
      //std::cout << Q_dir.norm() << " Qdir norm" << std::endl;

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
        coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};

        if (LorentzCorrection)
        {
          //double lambda = 1.0/wavenumber;
          // (sin(theta))^2 / wavelength^4
          float correct = float( sin_theta_squared * wavenumber*wavenumber*wavenumber*wavenumber );
          // Push the MDLeanEvent but correct the weight.
          out_events.push_back( MDE(float(it->weight()*correct), float(it->errorSquared()*correct*correct), center) );
        }
        else
        {
          // Push the MDLeanEvent with the same weight
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
        MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, (size_t)2e8);
      }

      // Add them to the MDEW
      ws->addEvents(out_events);
    }
    prog->reportIncrement(numEvents, "Adding Events");
  }


  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void ConvertToDiffractionMDWorkspace::exec()
  {
    Timer tim, timtotal;
    CPUTimer cputim, cputimtotal;

    // ---------------------- Extract properties --------------------------------------
    ClearInputWorkspace = getProperty("ClearInputWorkspace");
    Append = getProperty("Append");
    std::string OutputDimensions = getPropertyValue("OutputDimensions");
    LorentzCorrection = getProperty("LorentzCorrection");

    // -------- Input workspace -> convert to Event ------------------------------------
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    Workspace2D_sptr inWS2D = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);
    in_ws = boost::dynamic_pointer_cast<EventWorkspace>(inMatrixWS);
    if (!in_ws)
    {
      if (inWS2D)
      {
        // Convert from 2D to Event
        IAlgorithm_sptr alg = createSubAlgorithm("ConvertToEventWorkspace", 0.0, 0.1, true);
        alg->setProperty("InputWorkspace", inWS2D);
        alg->setProperty("GenerateMultipleEvents", false); // One event per bin by default
        alg->setPropertyValue("OutputWorkspace", getPropertyValue("InputWorkspace") + "_event");
        alg->executeAsSubAlg();
        in_ws = alg->getProperty("OutputWorkspace");
        if (!alg->isExecuted() || !in_ws)
          throw std::runtime_error("Error in ConvertToEventWorkspace. Cannot proceed.");
      }
      else
        throw std::invalid_argument("InputWorkspace must be either an EventWorkspace or a Workspace2D (which will get converted to events).");
    }


    // check the input units
    if (in_ws->getAxis(0)->unit()->unitID() != "TOF")
      throw std::invalid_argument("Input event workspace's X axis must be in TOF units.");

    // Try to get the output workspace
    IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    ws = boost::dynamic_pointer_cast<MDEventWorkspace<MDLeanEvent<3>,3> >( i_out );

    // Initalize the matrix to 3x3 identity
    mat = Kernel::Matrix<double>(3,3, true);

    // ----------------- Handle the type of output -------------------------------------

    std::string dimensionNames[3] = {"Q_lab_x", "Q_lab_y", "Q_lab_z"};
    std::string dimensionUnits = "Angstroms^-1";
    if (OutputDimensions == "Q (sample frame)")
    {
      // Set the matrix based on goniometer angles
      mat = in_ws->mutableRun().getGoniometerMatrix();
      // But we need to invert it, since we want to get the Q in the sample frame.
      mat.Invert();
      // Names
      dimensionNames[0] = "Q_sample_x";
      dimensionNames[1] = "Q_sample_y";
      dimensionNames[2] = "Q_sample_z";
    }
    else if (OutputDimensions == "HKL")
    {
      // Set the matrix based on UB etc.
      Kernel::Matrix<double> ub = in_ws->mutableSample().getOrientedLattice().getUB();
      Kernel::Matrix<double> gon = in_ws->mutableRun().getGoniometerMatrix();
      // As per Busing and Levy 1967, q_lab_frame = 2pi * Goniometer * UB * HKL
      // Therefore, HKL = (2*pi * Goniometer * UB)^-1 * q_lab_frame
      mat = gon * ub;
      mat.Invert();
      // Divide by 2 PI to account for our new convention, |Q| = 2pi / wl (December 2011, JZ)
      mat /= (2 * M_PI);
      dimensionNames[0] = "H";
      dimensionNames[1] = "K";
      dimensionNames[2] = "L";
      dimensionUnits = "lattice";
    }
    // Q in the lab frame is the default, so nothing special to do.

    if (ws && Append)
    {
      // Check that existing workspace dimensions make sense with the desired one (using the name)
      if (ws->getDimension(0)->getName() != dimensionNames[0])
        throw std::runtime_error("The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.");
    }



    // ------------------- Create the output workspace if needed ------------------------
    if (!ws || !Append)
    {
      // Create an output workspace with 3 dimensions.
      size_t nd = 3;
      i_out = MDEventFactory::CreateMDWorkspace(nd, "MDLeanEvent");
      ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(i_out);

      // ---------------- Get the extents -------------
      std::vector<double> extents = getProperty("Extents");
      // Replicate a single min,max into several
      if (extents.size() == 2)
        for (size_t d=1; d<nd; d++)
        {
          extents.push_back(extents[0]);
          extents.push_back(extents[1]);
        }
      if (extents.size() != nd*2)
        throw std::invalid_argument("You must specify either 2 or 6 extents (min,max).");


      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits, extents[d*2], extents[d*2+1], 10);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller, using the properties in BoxControllerSettingsAlgorithm
      BoxController_sptr bc = ws->getBoxController();
      this->setBoxController(bc);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
    }

    ws->splitBox();

    if (!ws)
      throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

    BoxController_sptr bc = ws->getBoxController();
    if (!bc)
      throw std::runtime_error("Output MDEventWorkspace does not have a BoxController!");

    // Copy ExperimentInfo (instrument, run, sample) to the output WS
    ExperimentInfo_sptr ei(in_ws->cloneExperimentInfo());
    uint16_t runIndex = ws->addExperimentInfo(ei);
    UNUSED_ARG(runIndex);


    // ------------------- Cache values that are common for all ---------------------------
    // Extract some parameters global to the instrument
    in_ws->getInstrument()->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);
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
    this->failedDetectorLookupCount = 0;
    size_t eventsAdded = 0;
    size_t lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
    if (DODEBUG) std::cout << cputim << ": initial setup. There are " << lastNumBoxes << " MDBoxes.\n";

    for (size_t wi=0; wi < in_ws->getNumberHistograms(); wi++)
    {
      // Equivalent of: this->convertEventList(wi);
      EventList & el = in_ws->getEventList(wi);

      // We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
      boost::function<void ()> func;
      switch (el.getEventType())
      {
      case TOF:
        func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<TofEvent>, &*this, static_cast<int>(wi));
        break;
      case WEIGHTED:
        func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<WeightedEvent>, &*this, static_cast<int>(wi));
        break;
      case WEIGHTED_NOTIME:
        func = boost::bind(&ConvertToDiffractionMDWorkspace::convertEventList<WeightedEventNoTime>, &*this, static_cast<int>(wi));
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
        if (DODEBUG) std::cout << cputim << ": Added tasks worth " << eventsAdded << " events. WorkspaceIndex " << wi << std::endl;
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

    if (this->failedDetectorLookupCount > 0)
    {
      if (this->failedDetectorLookupCount == 1)
      {
        g_log.warning()<<"Unable to find a detector for " << this->failedDetectorLookupCount << " spectrum. It has been skipped." << std::endl;
      }
      else
      {
        g_log.warning()<<"Unable to find detectors for " << this->failedDetectorLookupCount << " spectra. They have been skipped." << std::endl;
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
    ws->refreshCache();
    if (DODEBUG) std::cout << cputim << ": Performing the refreshCache().\n";

    //TODO: Centroid in parallel, maybe?
    ws->getBox()->refreshCentroid(NULL);
    if (DODEBUG) std::cout << cputim << ": Performing the refreshCentroid().\n";


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

