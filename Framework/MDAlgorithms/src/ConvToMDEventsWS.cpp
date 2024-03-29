// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/ConvToMDEventsWS.h"

#include "MantidMDAlgorithms/UnitsConversionHelper.h"

namespace Mantid::MDAlgorithms {
/**function converts particular list of events of type T into MD workspace and
 * adds these events to the workspace itself  */
template <class T> size_t ConvToMDEventsWS::convertEventList(size_t workspaceIndex) {

  const Mantid::DataObjects::EventList &el = m_EventWS->getSpectrum(workspaceIndex);
  size_t numEvents = el.getNumberEvents();
  if (numEvents == 0)
    return 0;

  // create local unit conversion class
  UnitsConversionHelper localUnitConv(m_UnitConversion);

  uint32_t detID = m_detID[workspaceIndex];
  uint16_t expInfoIndexLoc = m_ExpInfoIndex;

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
  std::vector<float> sig_err;             // array for signal and error.
  std::vector<uint16_t> expInfoIndex;     // Buffer for associated experiment-info index for each event
  std::vector<uint16_t> goniometer_index; // Buffer for goniometer index for each event
  std::vector<uint32_t> det_ids;          // Buffer of det Id-s for each event

  allCoord.reserve(this->m_NDims * numEvents);
  sig_err.reserve(2 * numEvents);
  expInfoIndex.reserve(numEvents);
  goniometer_index.reserve(numEvents);
  det_ids.reserve(numEvents);

  // This little dance makes the getting vector of events more general (since
  // you can't overload by return type).
  typename std::vector<T> const *events_ptr;
  getEventsFrom(el, events_ptr);
  const typename std::vector<T> &events = *events_ptr;

  // Iterators to start/end
  for (auto it = events.cbegin(); it != events.cend(); it++) {
    double val = localUnitConv.convertUnits(it->tof());
    double signal = it->weight();
    double errorSq = it->errorSquared();
    if (!m_QConverter->calcMatrixCoord(val, locCoord, signal, errorSq))
      continue; // skip ND outside the range

    sig_err.emplace_back(static_cast<float>(signal));
    sig_err.emplace_back(static_cast<float>(errorSq));
    expInfoIndex.emplace_back(expInfoIndexLoc);
    goniometer_index.emplace_back(0); // default value
    det_ids.emplace_back(detID);
    allCoord.insert(allCoord.end(), locCoord.begin(), locCoord.end());
  }

  // Add them to the MDEW
  size_t n_added_events = expInfoIndex.size();
  m_OutWSWrapper->addMDData(sig_err, expInfoIndex, goniometer_index, det_ids, allCoord, n_added_events);
  return n_added_events;
}

/** The method runs conversion for a single event list, corresponding to a
 * particular workspace index */
size_t ConvToMDEventsWS::conversionChunk(size_t workspaceIndex) {

  switch (m_EventWS->getSpectrum(workspaceIndex).getEventType()) {
  case Mantid::API::TOF:
    return this->convertEventList<Mantid::Types::Event::TofEvent>(workspaceIndex);
  case Mantid::API::WEIGHTED:
    return this->convertEventList<Mantid::DataObjects::WeightedEvent>(workspaceIndex);
  case Mantid::API::WEIGHTED_NOTIME:
    return this->convertEventList<Mantid::DataObjects::WeightedEventNoTime>(workspaceIndex);
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
size_t ConvToMDEventsWS::initialize(const MDWSDescription &WSD, std::shared_ptr<MDEventWSWrapper> inWSWrapper,
                                    bool ignoreZeros) {
  size_t numSpec = ConvToMDBase::initialize(WSD, inWSWrapper, ignoreZeros);

  m_EventWS = std::dynamic_pointer_cast<const DataObjects::EventWorkspace>(m_InWS2D);
  if (!m_EventWS)
    throw(std::logic_error(" ConvertToMDEventWS should work with defined event workspace"));

  // Record any special coordinate system known to the description.
  m_coordinateSystem = WSD.getCoordinateSystem();
  return numSpec;
}

void ConvToMDEventsWS::runConversion(API::Progress *pProgress) {

  // Get the box controller
  Mantid::API::BoxController_sptr bc = m_OutWSWrapper->pWorkspace()->getBoxController();

  // if any property dimension is outside of the data range requested, the job
  // is done;
  if (!m_QConverter->calcGenericVariables(m_Coord, m_NDims))
    return;

  appendEventsFromInputWS(pProgress, bc);

  pProgress->report();

  /// Set the special coordinate system flag on the output workspace.
  m_OutWSWrapper->pWorkspace()->setCoordinateSystem(m_coordinateSystem);
}

void ConvToMDEventsWS::appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  // Is the access to input events thread-safe?
  // bool MultiThreadedAdding = m_EventWS->threadSafe();
  // preprocessed detectors insure that each detector has its own spectra
  size_t lastNumBoxes = bc->getTotalNumMDBoxes();
  size_t nEventsInWS = m_OutWSWrapper->pWorkspace()->getNPoints();
  //--->>> Thread control stuff
  Kernel::ThreadSchedulerFIFO *ts(nullptr);

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
    pProgress->resetNumSteps(m_NSpectra, 0, 1);
  }
  Kernel::ThreadPool tp(ts, nThreads, new API::Progress(*pProgress));
  //<<<--  Thread control stuff

  size_t eventsAdded = 0;
  for (size_t wi = 0; wi < m_NSpectra; wi++) {

    size_t nConverted = conversionChunk(wi);
    eventsAdded += nConverted;
    nEventsInWS += nConverted;
    // Keep a running total of how many events we've added
    if (bc->shouldSplitBoxes(nEventsInWS, eventsAdded, lastNumBoxes)) {
      if (runMultithreaded) {
        // Now do all the splitting tasks
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          tp.joinAll();
      } else {
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(nullptr); // it is done this way as it is possible trying to do
                                                                 // single
                                                                 // threaded split more efficiently
      }
      // Count the new # of boxes.
      lastNumBoxes = m_OutWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
      eventsAdded = 0;
      pProgress->report(wi);
    }
  }
  // Do a final splitting of everything
  if (runMultithreaded) {
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
    tp.joinAll();
  } else {
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(nullptr);
  }

  // Recount totals at the end.
  m_OutWSWrapper->pWorkspace()->refreshCache();
}

} // namespace Mantid::MDAlgorithms
