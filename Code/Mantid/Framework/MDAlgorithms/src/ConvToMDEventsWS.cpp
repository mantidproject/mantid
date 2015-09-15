#include "MantidMDAlgorithms/ConvToMDEventsWS.h"

#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid {
namespace MDAlgorithms {
/**function converts particular list of events of type T into MD workspace and
 * adds these events to the workspace itself  */
template <class T>
size_t ConvToMDEventsWS::convertEventList(size_t workspaceIndex) {

  const Mantid::DataObjects::EventList &el =
      m_EventWS->getEventList(workspaceIndex);
  size_t numEvents = el.getNumberEvents();
  if (numEvents == 0)
    return 0;

  // create local unit conversion class
  UnitsConversionHelper localUnitConv(m_UnitConversion);

  uint32_t detID = m_detID[workspaceIndex];
  uint16_t runIndexLoc = m_RunIndex;

  std::vector<coord_t> locCoord(m_Coord);
  // set up unit conversion and calculate up all coordinates, which depend on
  // spectra index only
  if (!m_QConverter->calcYDepCoordinates(locCoord, workspaceIndex))
    return 0; // skip if any y outsize of the range of interest;
  localUnitConv.updateConversion(workspaceIndex);
  //
  // allocate temporary buffers for MD Events data
  // MD events coordinates buffer
  std::vector<coord_t> allCoord;
  std::vector<float> sig_err;      // array for signal and error.
  std::vector<uint16_t> run_index; // Buffer for run index for each event
  std::vector<uint32_t> det_ids;   // Buffer of det Id-s for each event

  allCoord.reserve(this->m_NDims * numEvents);
  sig_err.reserve(2 * numEvents);
  run_index.reserve(numEvents);
  det_ids.reserve(numEvents);

  // This little dance makes the getting vector of events more general (since
  // you can't overload by return type).
  typename std::vector<T> const *events_ptr;
  getEventsFrom(el, events_ptr);
  const typename std::vector<T> &events = *events_ptr;

  // Iterators to start/end
  typename std::vector<T>::const_iterator it = events.begin();
  typename std::vector<T>::const_iterator it_end = events.end();

  it = events.begin();
  for (; it != it_end; it++) {
    double val = localUnitConv.convertUnits(it->tof());
    double signal = it->weight();
    double errorSq = it->errorSquared();
    if (!m_QConverter->calcMatrixCoord(val, locCoord, signal, errorSq))
      continue; // skip ND outside the range

    sig_err.push_back(float(signal));
    sig_err.push_back(float(errorSq));
    run_index.push_back(runIndexLoc);
    det_ids.push_back(detID);
    allCoord.insert(allCoord.end(), locCoord.begin(), locCoord.end());
  }

  // Add them to the MDEW
  size_t n_added_events = run_index.size();
  m_OutWSWrapper->addMDData(sig_err, run_index, det_ids, allCoord,
                            n_added_events);
  return n_added_events;
}

/** The method runs conversion for a single event list, corresponding to a
 * particular workspace index */
size_t ConvToMDEventsWS::conversionChunk(size_t workspaceIndex) {

  switch (m_EventWS->getEventList(workspaceIndex).getEventType()) {
  case Mantid::API::TOF:
    return this->convertEventList<Mantid::DataObjects::TofEvent>(
        workspaceIndex);
  case Mantid::API::WEIGHTED:
    return this->convertEventList<Mantid::DataObjects::WeightedEvent>(
        workspaceIndex);
  case Mantid::API::WEIGHTED_NOTIME:
    return this->convertEventList<Mantid::DataObjects::WeightedEventNoTime>(
        workspaceIndex);
  default:
    throw std::runtime_error("EventList had an unexpected data type!");
  }
}

/** method sets up all internal variables necessary to convert from Event
Workspace to MDEvent workspace
@param WSD         -- the class describing the target MD workspace, sorurce
Event workspace and the transformations, necessary to perform on these
workspaces
@param inWSWrapper -- the class wrapping the target MD workspace
@param ignoreZeros  -- if zero value signals should be rejected
*/
size_t ConvToMDEventsWS::initialize(
    const MDWSDescription &WSD,
    boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
    bool ignoreZeros) {
  size_t numSpec = ConvToMDBase::initialize(WSD, inWSWrapper, ignoreZeros);

  m_EventWS =
      boost::dynamic_pointer_cast<const DataObjects::EventWorkspace>(m_InWS2D);
  if (!m_EventWS)
    throw(std::logic_error(
        " ConvertToMDEventWS should work with defined event workspace"));

  // Record any special coordinate system known to the description.
  m_coordinateSystem = WSD.getCoordinateSystem();
  return numSpec;
}

void ConvToMDEventsWS::runConversion(API::Progress *pProgress) {

  // Get the box controller
  Mantid::API::BoxController_sptr bc =
      m_OutWSWrapper->pWorkspace()->getBoxController();
  size_t lastNumBoxes = bc->getTotalNumMDBoxes();
  size_t nEventsInWS = m_OutWSWrapper->pWorkspace()->getNPoints();
  // Is the access to input events thread-safe?
  // bool MultiThreadedAdding = m_EventWS->threadSafe();
  // preprocessed detectors insure that each detector has its own spectra
  size_t nValidSpectra = m_NSpectra;

  //--->>> Thread control stuff
  Kernel::ThreadSchedulerFIFO *ts(NULL);

  int nThreads(m_NumThreads);
  if (nThreads < 0)
    nThreads = 0; // negative m_NumThreads correspond to all cores used, 0 no
                  // threads and positive number -- nThreads requested;
  bool runMultithreaded = false;
  if (m_NumThreads != 0) {
    runMultithreaded = true;
    // Create the thread pool that will run all of these. It will be deleted by
    // the threadpool
    ts = new Kernel::ThreadSchedulerFIFO();
    // it will initiate thread pool with number threads or machine's cores (0 in
    // tp constructor)
    pProgress->resetNumSteps(nValidSpectra, 0, 1);
  }
  Kernel::ThreadPool tp(ts, nThreads, new API::Progress(*pProgress));
  //<<<--  Thread control stuff

  // if any property dimension is outside of the data range requested, the job
  // is done;
  if (!m_QConverter->calcGenericVariables(m_Coord, m_NDims))
    return;

  size_t eventsAdded = 0;
  for (size_t wi = 0; wi < nValidSpectra; wi++) {

    size_t nConverted = this->conversionChunk(wi);
    eventsAdded += nConverted;
    nEventsInWS += nConverted;
    // Give this task to the scheduler
    //%double cost = double(el.getNumberEvents());
    // ts->push( new FunctionTask( func, cost) );

    // Keep a running total of how many events we've added
    if (bc->shouldSplitBoxes(nEventsInWS, eventsAdded, lastNumBoxes)) {
      if (runMultithreaded) {
        // Do all the adding tasks
        tp.joinAll();
        // Now do all the splitting tasks
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          tp.joinAll();
      } else {
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(
            NULL); // it is done this way as it is possible trying to do single
                   // threaded split more efficiently
      }
      // Count the new # of boxes.
      lastNumBoxes = m_OutWSWrapper->pWorkspace()
                         ->getBoxController()
                         ->getTotalNumMDBoxes();
      eventsAdded = 0;
      pProgress->report(wi);
    }
  }
  // Do a final splitting of everything
  if (runMultithreaded) {
    tp.joinAll();
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
    tp.joinAll();
  } else {
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(NULL);
  }

  // Recount totals at the end.
  m_OutWSWrapper->pWorkspace()->refreshCache();
  // m_OutWSWrapper->refreshCentroid();
  pProgress->report();

  /// Set the special coordinate system flag on the output workspace.
  m_OutWSWrapper->pWorkspace()->setCoordinateSystem(m_coordinateSystem);
}

} // endNamespace DataObjects
} // endNamespace Mantid
