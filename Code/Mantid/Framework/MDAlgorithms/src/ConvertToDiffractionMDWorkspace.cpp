#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/ProgressText.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace MDAlgorithms {

bool DODEBUG = true;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToDiffractionMDWorkspace)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ConvertToDiffractionMDWorkspace::ConvertToDiffractionMDWorkspace()
    : ClearInputWorkspace(false), // imput workspace should be left untouched
      OneEventPerBin(false),      // it is very expensive otherwise
      Append(true), // append data to existing target MD workspace if one exist
      LorentzCorrection(false), // not doing Lorents
      l1(1.), beamline_norm(1.), failedDetectorLookupCount(0),
      m_extentsMin(NULL),
      m_extentsMax(NULL) // will be allocated in exec using nDims
{}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConvertToDiffractionMDWorkspace::init() {
  // Input units must be TOF
  auto validator = boost::make_shared<API::WorkspaceUnitValidator>("TOF");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input, validator),
                  "An input workspace in time-of-flight. If you specify a "
                  "Workspace2D, it gets converted to "
                  "an EventWorkspace using ConvertToEventWorkspace.");

  declareProperty(new WorkspaceProperty<IMDEventWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "Name of the output MDEventWorkspace. If the workspace "
                  "already exists, then the events will be added to it.");
  declareProperty(
      new PropertyWithValue<bool>("Append", false, Direction::Input),
      "Append events to the output workspace. The workspace is replaced if "
      "unchecked.");
  declareProperty(new PropertyWithValue<bool>("ClearInputWorkspace", false,
                                              Direction::Input),
                  "Clear the events from the input workspace during "
                  "conversion, to save memory.");

  declareProperty(
      new PropertyWithValue<bool>("OneEventPerBin", false, Direction::Input),
      "Use the histogram representation (event for event workspaces).\n"
      "One MDEvent will be created for each histogram bin (even empty ones).\n"
      "Warning! This can use signficantly more memory!");

  std::vector<std::string> propOptions;
  propOptions.push_back("Q (lab frame)");
  propOptions.push_back("Q (sample frame)");
  propOptions.push_back("HKL");
  declareProperty(
      "OutputDimensions", "Q (lab frame)",
      boost::make_shared<StringListValidator>(propOptions),
      "What will be the dimensions of the output workspace?\n"
      "  Q (lab frame): Wave-vector change of the lattice in the lab frame.\n"
      "  Q (sample frame): Wave-vector change of the lattice in the frame of "
      "the sample (taking out goniometer rotation).\n"
      "  HKL: Use the sample's UB matrix to convert to crystal's HKL indices.");

  declareProperty(
      new PropertyWithValue<bool>("LorentzCorrection", false, Direction::Input),
      "Correct the weights of events with by multiplying by the Lorentz "
      "formula: sin(theta)^2 / lambda^4");

  // Box controller properties. These are the defaults
  this->initBoxControllerProps("2" /*SplitInto*/, 1500 /*SplitThreshold*/,
                               20 /*MaxRecursionDepth*/);

  declareProperty(
      new PropertyWithValue<int>("MinRecursionDepth", 0),
      "Optional. If specified, then all the boxes will be split to this "
      "minimum recursion depth. 1 = one level of splitting, etc.\n"
      "Be careful using this since it can quickly create a huge number of "
      "boxes = (SplitInto ^ (MinRercursionDepth x NumDimensions)).\n"
      "But setting this property equal to MaxRecursionDepth property is "
      "necessary if one wants to generate multiple file based workspaces in "
      "order to merge them later\n");
  setPropertyGroup("MinRecursionDepth", getBoxSettingsGroupName());

  std::vector<double> extents(2, 0);
  extents[0] = -50;
  extents[1] = +50;
  declareProperty(new ArrayProperty<double>("Extents", extents),
                  "A comma separated list of min, max for each dimension,\n"
                  "specifying the extents of each dimension. Optional, default "
                  "+-50 in each dimension.");
  setPropertyGroup("Extents", getBoxSettingsGroupName());
}

/// Our MDLeanEvent dimension
typedef MDEvents::MDLeanEvent<3> MDE;

//----------------------------------------------------------------------------------------------
/** Convert one spectrum to MDEvents.
 * Depending on options, it uses the histogram view or the
 * pure event view.
 * Then another method converts to 3D q-space and adds it to the
 *MDEventWorkspace
 *
 * @param workspaceIndex :: index into the workspace
 */
void ConvertToDiffractionMDWorkspace::convertSpectrum(int workspaceIndex) {
  if (m_inEventWS && !OneEventPerBin) {
    // ---------- Convert events directly -------------------------
    EventList &el = m_inEventWS->getEventList(workspaceIndex);

    // Call the right templated function
    switch (el.getEventType()) {
    case TOF:
      this->convertEventList<TofEvent>(workspaceIndex, el);
      break;
    case WEIGHTED:
      this->convertEventList<WeightedEvent>(workspaceIndex, el);
      break;
    case WEIGHTED_NOTIME:
      this->convertEventList<WeightedEventNoTime>(workspaceIndex, el);
      break;
    default:
      throw std::runtime_error("EventList had an unexpected data type!");
    }
  } else {
    // ----- Workspace2D, or use the Histogram representation of EventWorkspace
    // ------------
    // Construct a new event list
    EventList el;

    // Create the events using the bins
    const ISpectrum *inSpec = m_inWS->getSpectrum(workspaceIndex);
    // If OneEventPerBin, generate exactly 1 event per bin, including zeros.
    // If !OneEventPerBin, generate up to 10 events per bin, excluding zeros
    el.createFromHistogram(
        inSpec, OneEventPerBin /* Generate zeros */,
        !OneEventPerBin /* Multiple events */,
        (OneEventPerBin ? 1 : 10) /* Max of this many events per bin */);

    // Perform the conversion on this temporary event list
    this->convertEventList<WeightedEventNoTime>(workspaceIndex, el);
  }
}

//----------------------------------------------------------------------------------------------
/** Convert an event list to 3D q-space and add it to the MDEventWorkspace
 *
 * @tparam T :: the type of event in the input EventList (TofEvent,
 *WeightedEvent, etc.)
 * @param workspaceIndex :: the workspace index
 * @param el :: reference to the event list
 */
template <class T>
void ConvertToDiffractionMDWorkspace::convertEventList(int workspaceIndex,
                                                       EventList &el) {
  size_t numEvents = el.getNumberEvents();
  MDEvents::MDBoxBase<MDEvents::MDLeanEvent<3>, 3> *box = ws->getBox();

  // Get the position of the detector there.
  const std::set<detid_t> &detectors = el.getDetectorIDs();
  if (!detectors.empty()) {
    // Get the detector (might be a detectorGroup for multiple detectors)
    // or might return an exception if the detector is not in the instrument
    // definition
    IDetector_const_sptr det;
    try {
      det = m_inWS->getDetector(workspaceIndex);
    } catch (Exception::NotFoundError &) {
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
    //  = input beam direction (normalized to 1) - output beam direction
    //  (normalized to 1)
    V3D Q_dir_lab_frame = beamDir - detDir;

    // Multiply by the rotation matrix to convert to Q in the sample frame (take
    // out goniometer rotation)
    // (or to HKL, if that's what the matrix is)
    V3D Q_dir = mat * Q_dir_lab_frame;

    // For speed we extract the components.
    coord_t Q_dir_x = coord_t(Q_dir.X());
    coord_t Q_dir_y = coord_t(Q_dir.Y());
    coord_t Q_dir_z = coord_t(Q_dir.Z());

    // For lorentz correction, calculate  sin(theta))^2
    double sin_theta_squared = 0;
    if (LorentzCorrection) {
      // Scattering angle = 2 theta = angle between neutron beam direction and
      // the detector (scattering) direction
      // The formula for Lorentz Correction is sin(theta), i.e. sin(half the
      // scattering angle)
      double theta = detDir.angle(beamDir) / 2.0;
      sin_theta_squared = sin(theta);
      sin_theta_squared = sin_theta_squared * sin_theta_squared; // square it
    }

    /** Constant that you divide by tof (in usec) to get wavenumber in ang^-1 :
     * Wavenumber (in ang^-1) =  (PhysicalConstants::NeutronMass * distance) /
     * ((tof (in usec) * 1e-6) * PhysicalConstants::h_bar) * 1e-10; */
    const double wavenumber_in_angstrom_times_tof_in_microsec =
        (PhysicalConstants::NeutronMass * distance * 1e-10) /
        (1e-6 * PhysicalConstants::h_bar);

    // PARALLEL_CRITICAL( convert_tester_output ) { std::cout << "Spectrum " <<
    // el.getSpectrumNo() << " beamDir = " << beamDir << " detDir = " << detDir
    // << " Q_dir = " << Q_dir << " conversion factor " <<
    // wavenumber_in_angstrom_times_tof_in_microsec << std::endl;  }

    // g_log.information() << wi << " : " << el.getNumberEvents() << " events.
    // Pos is " << detPos << std::endl;
    // g_log.information() << Q_dir.norm() << " Qdir norm" << std::endl;

    // This little dance makes the getting vector of events more general (since
    // you can't overload by return type).
    typename std::vector<T> *events_ptr;
    getEventsFrom(el, events_ptr);
    typename std::vector<T> &events = *events_ptr;

    // Iterators to start/end
    typename std::vector<T>::iterator it = events.begin();
    typename std::vector<T>::iterator it_end = events.end();

    for (; it != it_end; it++) {
      // Get the wavenumber in ang^-1 using the previously calculated constant.
      coord_t wavenumber =
          coord_t(wavenumber_in_angstrom_times_tof_in_microsec / it->tof());

      // Q vector = K_final - K_initial = wavenumber * (output_direction -
      // input_direction)
      coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber,
                           Q_dir_z * wavenumber};

      // Check that the event is within bounds
      if (center[0] < m_extentsMin[0] || center[0] >= m_extentsMax[0])
        continue;
      if (center[1] < m_extentsMin[1] || center[1] >= m_extentsMax[1])
        continue;
      if (center[2] < m_extentsMin[2] || center[2] >= m_extentsMax[2])
        continue;

      if (LorentzCorrection) {
        // double lambda = 1.0/wavenumber;
        // (sin(theta))^2 / wavelength^4
        float correct = float(sin_theta_squared * wavenumber * wavenumber *
                              wavenumber * wavenumber);
        // Push the MDLeanEvent but correct the weight.
        box->addEvent(MDE(float(it->weight() * correct),
                          float(it->errorSquared() * correct * correct),
                          center));
      } else {
        // Push the MDLeanEvent with the same weight
        box->addEvent(
            MDE(float(it->weight()), float(it->errorSquared()), center));
      }
    }

    // Clear out the EventList to save memory
    if (ClearInputWorkspace) {
      // Track how much memory you cleared
      size_t memoryCleared = el.getMemorySize();
      // Clear it now
      el.clear();
      // For Linux with tcmalloc, make sure memory goes back, if you've cleared
      // 200 Megs
      MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared,
                                                               (size_t)2e8);
    }
  }
  prog->reportIncrement(numEvents, "Adding Events");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConvertToDiffractionMDWorkspace::exec() {
  Timer tim, timtotal;
  CPUTimer cputim, cputimtotal;

  // ---------------------- Extract properties
  // --------------------------------------
  ClearInputWorkspace = getProperty("ClearInputWorkspace");
  Append = getProperty("Append");
  std::string OutputDimensions = getPropertyValue("OutputDimensions");
  LorentzCorrection = getProperty("LorentzCorrection");
  OneEventPerBin = getProperty("OneEventPerBin");

  // -------- Input workspace -> convert to Event
  // ------------------------------------
  m_inWS = getProperty("InputWorkspace");
  Workspace2D_sptr m_InWS2D = boost::dynamic_pointer_cast<Workspace2D>(m_inWS);
  if (LorentzCorrection) {
    API::Run &run = m_inWS->mutableRun();
    if (run.hasProperty("LorentzCorrection")) {
      Kernel::Property *prop = run.getProperty("LorentzCorrection");
      bool lorentzDone = boost::lexical_cast<bool, std::string>(prop->value());
      if (lorentzDone) {
        LorentzCorrection = false;
        g_log.warning() << "Lorentz Correction was already done for this "
                           "workspace.  LorentzCorrection was changed to false."
                        << std::endl;
      }
    }
  }

  m_inEventWS = boost::dynamic_pointer_cast<EventWorkspace>(m_inWS);

  // check the input units
  if (m_inWS->getAxis(0)->unit()->unitID() != "TOF")
    throw std::invalid_argument(
        "Input event workspace's X axis must be in TOF units.");

  // Try to get the output workspace
  IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
  ws = boost::dynamic_pointer_cast<
      MDEvents::MDEventWorkspace<MDEvents::MDLeanEvent<3>, 3>>(i_out);

  // Initalize the matrix to 3x3 identity
  mat = Kernel::Matrix<double>(3, 3, true);

  // ----------------- Handle the type of output
  // -------------------------------------

  std::string dimensionNames[3] = {"Q_lab_x", "Q_lab_y", "Q_lab_z"};
  std::string dimensionUnits = "Angstroms^-1";
  Mantid::Kernel::SpecialCoordinateSystem coordinateSystem = Mantid::Kernel::QLab;
  if (OutputDimensions == "Q (sample frame)") {
    // Set the matrix based on goniometer angles
    mat = m_inWS->mutableRun().getGoniometerMatrix();
    // But we need to invert it, since we want to get the Q in the sample frame.
    mat.Invert();
    // Names
    dimensionNames[0] = "Q_sample_x";
    dimensionNames[1] = "Q_sample_y";
    dimensionNames[2] = "Q_sample_z";
    coordinateSystem = Mantid::Kernel::QSample;
  } else if (OutputDimensions == "HKL") {
    // Set the matrix based on UB etc.
    Kernel::Matrix<double> ub =
        m_inWS->mutableSample().getOrientedLattice().getUB();
    Kernel::Matrix<double> gon = m_inWS->mutableRun().getGoniometerMatrix();
    // As per Busing and Levy 1967, q_lab_frame = 2pi * Goniometer * UB * HKL
    // Therefore, HKL = (2*pi * Goniometer * UB)^-1 * q_lab_frame
    mat = gon * ub;
    mat.Invert();
    // Divide by 2 PI to account for our new convention, |Q| = 2pi / wl
    // (December 2011, JZ)
    mat /= (2 * M_PI);
    dimensionNames[0] = "H";
    dimensionNames[1] = "K";
    dimensionNames[2] = "L";
    dimensionUnits = "lattice";
    coordinateSystem = Mantid::Kernel::HKL;
  }
  // Q in the lab frame is the default, so nothing special to do.

  if (ws && Append) {
    // Check that existing workspace dimensions make sense with the desired one
    // (using the name)
    if (ws->getDimension(0)->getName() != dimensionNames[0])
      throw std::runtime_error("The existing MDEventWorkspace " +
                               ws->getName() +
                               " has different dimensions than were requested! "
                               "Either give a different name for the output, "
                               "or change the OutputDimensions parameter.");
  }

  // ------------------- Create the output workspace if needed
  // ------------------------
  if (!ws || !Append) {
    // Create an output workspace with 3 dimensions.
    size_t nd = 3;
    i_out = MDEvents::MDEventFactory::CreateMDWorkspace(nd, "MDLeanEvent");
    ws = boost::dynamic_pointer_cast<MDEvents::MDEventWorkspace3Lean>(i_out);

    // ---------------- Get the extents -------------
    std::vector<double> extents = getProperty("Extents");
    // Replicate a single min,max into several
    if (extents.size() == 2)
      for (size_t d = 1; d < nd; d++) {
        extents.push_back(extents[0]);
        extents.push_back(extents[1]);
      }
    if (extents.size() != nd * 2)
      throw std::invalid_argument(
          "You must specify either 2 or 6 extents (min,max).");

    // Give all the dimensions
    for (size_t d = 0; d < nd; d++) {
      MDHistoDimension *dim = new MDHistoDimension(
          dimensionNames[d], dimensionNames[d], dimensionUnits,
          static_cast<coord_t>(extents[d * 2]),
          static_cast<coord_t>(extents[d * 2 + 1]), 10);
      ws->addDimension(MDHistoDimension_sptr(dim));
    }
    ws->initialize();

    // Build up the box controller, using the properties in
    // BoxControllerSettingsAlgorithm
    BoxController_sptr bc = ws->getBoxController();
    this->setBoxController(bc, m_inWS->getInstrument());
    // We always want the box to be split (it will reject bad ones)
    ws->splitBox();

    // Perform minimum recursion depth splitting
    int minDepth = this->getProperty("MinRecursionDepth");
    int maxDepth = this->getProperty("MaxRecursionDepth");
    if (minDepth > maxDepth)
      throw std::invalid_argument(
          "MinRecursionDepth must be <= MaxRecursionDepth ");
    ws->setMinRecursionDepth(size_t(minDepth));
  }

  ws->splitBox();

  if (!ws)
    throw std::runtime_error("Error creating a 3D MDEventWorkspace!");

  BoxController_sptr bc = ws->getBoxController();
  if (!bc)
    throw std::runtime_error(
        "Output MDEventWorkspace does not have a BoxController!");

  // Cache the extents for speed.
  m_extentsMin = new coord_t[3];
  m_extentsMax = new coord_t[3];
  for (size_t d = 0; d < 3; d++) {
    m_extentsMin[d] = ws->getDimension(d)->getMinimum();
    m_extentsMax[d] = ws->getDimension(d)->getMaximum();
  }

  // Copy ExperimentInfo (instrument, run, sample) to the output WS
  ExperimentInfo_sptr ei(m_inWS->cloneExperimentInfo());
  uint16_t runIndex = ws->addExperimentInfo(ei);
  UNUSED_ARG(runIndex);

  // ------------------- Cache values that are common for all
  // ---------------------------
  // Extract some parameters global to the instrument
  m_inWS->getInstrument()->getInstrumentParameters(l1, beamline, beamline_norm,
                                                   samplePos);
  beamline_norm = beamline.norm();
  beamDir = beamline / beamline.norm();

  // To get all the detector ID's
  m_inWS->getInstrument()->getDetectors(allDetectors);

  // Estimate the number of events in the final workspace
  size_t totalEvents = m_inWS->size();
  if (m_inEventWS && !OneEventPerBin)
    totalEvents = m_inEventWS->getNumberEvents();
  prog = boost::make_shared<Progress>(this, 0, 1.0, totalEvents);

  // Is the addition of events thread-safe?
  bool MultiThreadedAdding = m_inWS->threadSafe();

  // Create the thread pool that will run all of these.
  ThreadScheduler *ts = new ThreadSchedulerFIFO();
  ThreadPool tp(ts, 0);

  // To track when to split up boxes
  this->failedDetectorLookupCount = 0;
  size_t eventsAdded = 0;
  size_t approxEventsInOutput = 0;
  size_t lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
  if (DODEBUG)
    g_log.information() << cputim << ": initial setup. There are "
                        << lastNumBoxes << " MDBoxes.\n";

  for (size_t wi = 0; wi < m_inWS->getNumberHistograms(); wi++) {
    // Get an idea of how many events we'll be adding
    size_t eventsAdding = m_inWS->blocksize();
    if (m_inEventWS && !OneEventPerBin)
      eventsAdding = m_inEventWS->getEventList(wi).getNumberEvents();

    if (MultiThreadedAdding) {
      // Equivalent to calling "this->convertSpectrum(wi)"
      boost::function<void()> func =
          boost::bind(&ConvertToDiffractionMDWorkspace::convertSpectrum, &*this,
                      static_cast<int>(wi));
      // Give this task to the scheduler
      double cost = static_cast<double>(eventsAdding);
      ts->push(new FunctionTask(func, cost));
    } else {
      // Not thread-safe. Just add right now
      this->convertSpectrum(static_cast<int>(wi));
    }

    // Keep a running total of how many events we've added
    eventsAdded += eventsAdding;
    approxEventsInOutput += eventsAdding;

    if (bc->shouldSplitBoxes(approxEventsInOutput, eventsAdded, lastNumBoxes)) {
      if (DODEBUG)
        g_log.information() << cputim << ": Added tasks worth " << eventsAdded
                            << " events. WorkspaceIndex " << wi << std::endl;
      // Do all the adding tasks
      tp.joinAll();
      if (DODEBUG)
        g_log.information() << cputim
                            << ": Performing the addition of these events.\n";

      // Now do all the splitting tasks
      ws->splitAllIfNeeded(ts);
      if (ts->size() > 0)
        prog->doReport("Splitting Boxes");
      tp.joinAll();

      // Count the new # of boxes.
      lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
      if (DODEBUG)
        g_log.information() << cputim
                            << ": Performing the splitting. There are now "
                            << lastNumBoxes << " boxes.\n";
      eventsAdded = 0;
    }
  }

  if (this->failedDetectorLookupCount > 0) {
    if (this->failedDetectorLookupCount == 1)
      g_log.warning() << "Unable to find a detector for "
                      << this->failedDetectorLookupCount
                      << " spectrum. It has been skipped." << std::endl;
    else
      g_log.warning() << "Unable to find detectors for "
                      << this->failedDetectorLookupCount
                      << " spectra. They have been skipped." << std::endl;
  }

  if (DODEBUG)
    g_log.information() << cputim << ": We've added tasks worth " << eventsAdded
                        << " events.\n";

  tp.joinAll();
  if (DODEBUG)
    g_log.information() << cputim
                        << ": Performing the FINAL addition of these events.\n";

  // Do a final splitting of everything
  ws->splitAllIfNeeded(ts);
  tp.joinAll();
  if (DODEBUG)
    g_log.information()
        << cputim << ": Performing the FINAL splitting of boxes. There are now "
        << ws->getBoxController()->getTotalNumMDBoxes() << " boxes\n";

  // Recount totals at the end.
  cputim.reset();
  ws->refreshCache();
  if (DODEBUG)
    g_log.information() << cputim << ": Performing the refreshCache().\n";

  // TODO: Centroid in parallel, maybe?
  // ws->getBox()->refreshCentroid(NULL);
  // if (DODEBUG) g_log.information() << cputim << ": Performing the
  // refreshCentroid().\n";

  if (DODEBUG) {
    g_log.information() << "Workspace has " << ws->getNPoints()
                        << " events. This took " << cputimtotal
                        << " in total.\n";
    std::vector<std::string> stats = ws->getBoxControllerStats();
    for (size_t i = 0; i < stats.size(); ++i)
      g_log.information() << stats[i] << "\n";
    g_log.information() << std::endl;
  }

  // Set the special coordinate system.
  ws->setCoordinateSystem(coordinateSystem);

  // Save the output
  setProperty("OutputWorkspace",
              boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

  // Clean up
  delete[] m_extentsMin;
  delete[] m_extentsMax;
}

} // namespace Mantid
} // namespace MDEvents
