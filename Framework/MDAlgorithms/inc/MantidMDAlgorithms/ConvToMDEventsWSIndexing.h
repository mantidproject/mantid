// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_
#define MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_

#include "MantidMDAlgorithms/ConvToMDEventsWS.h"
#include "MantidMDAlgorithms/MDEventTreeBuilder.h"
#include <mutex>
#include <queue>
#include <thread>

namespace Mantid {
// Forward declarations
namespace API {
class Progress;
}
namespace MDAlgorithms {
/**
 * This class creates the MDWorkspace from the collection of
 * ToF events: converts to the MD events with proper Nd
 * coordinate and than assigns the groups of them to the
 * spatial tree-like box structure. The difference with
 * the ConvToMDEventsWS is in using the spatial index (Morton
 * numbers) for speeding up the procedure.
 */
class ConvToMDEventsWSIndexing : public ConvToMDEventsWS {
  enum MD_EVENT_TYPE { LEAN, REGULAR, NONE };

  size_t initialize(const MDWSDescription &WSD,
                    boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
                    bool ignoreZeros) override;
  // Interface function
  void appendEventsFromInputWS(API::Progress *pProgress,
                               const API::BoxController_sptr &bc) override;

public:
  template <typename T>
  static bool isSplitValid(const std::vector<T> &split_into) {
    bool validSplitInfo = !split_into.empty();
    if (validSplitInfo) {
      const T &n = split_into[0];
      validSplitInfo &= (n > 1 && ((n & (n - 1)) == 0));
      if (validSplitInfo)
        validSplitInfo &= all_of(split_into.begin(), split_into.end(),
                                 [&n](T i) { return i == n; });
    }
    return validSplitInfo;
  }

private:
  // Returns number of workers for parallel parts
  int numWorkers() {
    return this->m_NumThreads < 0 ? PARALLEL_GET_MAX_THREADS
                                  : std::max(1, this->m_NumThreads);
  }

  template <size_t ND> MD_EVENT_TYPE mdEventType();

  // Wrapper to have the proper functions, for Nd in range 2 to maxDim
  template <size_t maxDim>
  void appendEventsFromInputWS(API::Progress *pProgress,
                               const API::BoxController_sptr &bc);

  // Wrapper for ToF events of different types, number of dims, MD event type
  template <typename EventType, size_t ND, template <size_t> class MDEventType>
  void appendEvents(API::Progress *pProgress,
                    const API::BoxController_sptr &bc);

  // Specialization for ToF events of different types
  template <size_t ND, template <size_t> class MDEventType>
  void appendEvents(API::Progress *pProgress,
                    const API::BoxController_sptr &bc);

  // Specilization for MD event types
  template <size_t ND>
  void appendEvents(API::Progress *pProgress,
                    const API::BoxController_sptr &bc);

  template <typename EventType, size_t ND, template <size_t> class MDEventType>
  std::vector<MDEventType<ND>> convertEvents();

  template <size_t ND, template <size_t> class MDEventType>
  struct MDEventMaker {
    static MDEventType<ND> makeMDEvent(const double &sig, const double &err,
                                       const uint16_t &run_index,
                                       const uint32_t &det_id, coord_t *coord) {
      return MDEventType<ND>(sig, err, run_index, det_id, coord);
    }
  };
};

/*-------------------------------definitions-------------------------------------*/

template <typename EventType, size_t ND, template <size_t> class MDEventType>
std::vector<MDEventType<ND>> ConvToMDEventsWSIndexing::convertEvents() {
  std::vector<MDEventType<ND>> mdEvents;
  mdEvents.reserve(m_EventWS->getNumberEvents());

  const auto &pws = m_OutWSWrapper->pWorkspace();
  std::array<std::pair<coord_t, coord_t>, ND> bounds;
  for (size_t ax = 0; ax < ND; ++ax) {
    bounds[ax] = std::make_pair(pws->getDimension(ax)->getMinimum(),
                                pws->getDimension(ax)->getMaximum());
  }

  std::vector<MDTransf_sptr> qConverters;
  for (int i = 0; i < numWorkers(); ++i)
    qConverters.emplace_back(m_QConverter->clone());
#pragma omp parallel for num_threads(numWorkers())
  for (int workspaceIndex = 0; workspaceIndex < static_cast<int>(m_NSpectra);
       ++workspaceIndex) {
    const Mantid::DataObjects::EventList &el =
        m_EventWS->getSpectrum(workspaceIndex);

    size_t numEvents = el.getNumberEvents();
    if (numEvents == 0)
      continue;

    // create local unit conversion class
    UnitsConversionHelper localUnitConv(m_UnitConversion);
    // create local QConverter
    MDTransf_sptr localQConverter = qConverters[PARALLEL_THREAD_NUMBER];
    int32_t detID = m_detID[workspaceIndex];
    uint16_t runIndexLoc = m_RunIndex;

    std::vector<coord_t> locCoord(ND);
    // set up unit conversion and calculate up all coordinates, which depend on
    // spectra index only
    if (!localQConverter->calcYDepCoordinates(locCoord, workspaceIndex))
      continue; // skip if any y outsize of the range of interest;
    localUnitConv.updateConversion(workspaceIndex);
    // This little dance makes the getting vector of events more general (since
    // you can't overload by return type).
    typename std::vector<EventType> const *events_ptr;
    getEventsFrom(el, events_ptr);
    const typename std::vector<EventType> &events = *events_ptr;
    std::vector<MDEventType<ND>> mdEventsForSpectrum;
    // Iterators to start/end
    for (const auto &event : events) {
      double val = localUnitConv.convertUnits(event.tof());
      double signal = event.weight();
      double errorSq = event.errorSquared();

      if (!localQConverter->calcMatrixCoord(val, locCoord, signal, errorSq))
        continue; // skip ND outside the range

      mdEventsForSpectrum.emplace_back(
          MDEventMaker<ND, MDEventType>::makeMDEvent(
              signal, errorSq, runIndexLoc, detID, &locCoord[0]));

      // Filter events before adding to the ndEvents vector to add in workspace
      // The bounds of the resulting WS have to be already defined
      bool isInOutWSBox = true;
      for (size_t ax = 0; ax < ND; ++ax) {
        const coord_t &coord{mdEventsForSpectrum.back().getCenter(ax)};
        if (coord < bounds[ax].first || coord > bounds[ax].second)
          isInOutWSBox = false;
      }

      if (!isInOutWSBox)
        mdEventsForSpectrum.pop_back();
    }

#pragma omp critical
    {
      /* Add to event list */
      mdEvents.insert(mdEvents.cend(), mdEventsForSpectrum.begin(),
                      mdEventsForSpectrum.end());
    }
  }
  return mdEvents;
}

template <typename EventType, size_t ND, template <size_t> class MDEventType>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress,
                                            const API::BoxController_sptr &bc) {
  bc->clearBoxesCounter(1);
  bc->clearGridBoxesCounter(0);
  pProgress->resetNumSteps(2, 0, 1);

  std::vector<MDEventType<ND>> mdEvents =
      convertEvents<EventType, ND, MDEventType>();

  morton_index::MDSpaceBounds<ND> space;
  const auto &pws = m_OutWSWrapper->pWorkspace();
  for (size_t ax = 0; ax < ND; ++ax) {
    space(ax, 0) = pws->getDimension(ax)->getMinimum();
    space(ax, 1) = pws->getDimension(ax)->getMaximum();
  }

  pProgress->report(0);

  auto nThreads = numWorkers();
  using EventDistributor =
      MDEventTreeBuilder<ND, MDEventType,
                         typename std::vector<MDEventType<ND>>::iterator>;
  EventDistributor distributor(nThreads, mdEvents.size() / nThreads / 10, bc,
                               space);

  auto rootAndErr = distributor.distribute(mdEvents);
  m_OutWSWrapper->pWorkspace()->setBox(rootAndErr.root);
  rootAndErr.root->calculateGridCaches();

  std::stringstream ss;
  ss << rootAndErr.err;
  g_Log.information("Error with using Morton indexes is:\n" + ss.str());
  pProgress->report(1);
}

// Specialization for ToF events of different types
template <size_t ND, template <size_t> class MDEventType>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress,
                                            const API::BoxController_sptr &bc) {
  switch (m_EventWS->getSpectrum(0).getEventType()) {
  case Mantid::API::TOF:
    appendEvents<Mantid::Types::Event::TofEvent, ND, MDEventType>(pProgress,
                                                                  bc);
    break;
  case Mantid::API::WEIGHTED:
    appendEvents<Mantid::DataObjects::WeightedEvent, ND, MDEventType>(pProgress,
                                                                      bc);
    break;
  case Mantid::API::WEIGHTED_NOTIME:
    appendEvents<Mantid::DataObjects::WeightedEventNoTime, ND, MDEventType>(
        pProgress, bc);
    break;
  default:
    throw std::runtime_error(
        "Events in event workspace had an unexpected data type!");
  }
}

// Specilization for MD event types
template <size_t ND>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress,
                                            const API::BoxController_sptr &bc) {
  switch (mdEventType<ND>()) {
  case LEAN:
    appendEvents<ND, DataObjects::MDLeanEvent>(pProgress, bc);
    break;
  case REGULAR:
    appendEvents<ND, DataObjects::MDEvent>(pProgress, bc);
    break;
  default:
    throw std::runtime_error(
        "MD events in md event workspace had an unexpected data type!");
  }
}

// Wrapper to have the proper functions, for Nd in range 2 to maxDim
template <size_t maxDim>
void ConvToMDEventsWSIndexing::appendEventsFromInputWS(
    API::Progress *pProgress, const API::BoxController_sptr &bc) {
  auto ndim = m_OutWSWrapper->nDimensions();
  if (ndim < 2)
    throw std::runtime_error("Can't convert to MD workspace with dims " +
                             std::to_string(ndim) + "less than 2");
  if (ndim > maxDim)
    return;
  if (ndim == maxDim) {
    appendEvents<maxDim>(pProgress, bc);
    return;
  } else
    appendEventsFromInputWS<maxDim - 1>(pProgress, bc);
}

template <size_t ND>
struct ConvToMDEventsWSIndexing::MDEventMaker<
    ND, Mantid::DataObjects::MDLeanEvent> {
  static Mantid::DataObjects::MDLeanEvent<ND>
  makeMDEvent(const double &sig, const double &err, const uint16_t,
              const uint32_t, coord_t *coord) {
    return Mantid::DataObjects::MDLeanEvent<ND>(sig, err, coord);
  }
};

template <size_t ND>
ConvToMDEventsWSIndexing::MD_EVENT_TYPE
ConvToMDEventsWSIndexing::mdEventType() {
  if (dynamic_cast<DataObjects::MDEventWorkspace<DataObjects::MDEvent<ND>, ND>
                       *>(m_OutWSWrapper->pWorkspace().get()))
    return REGULAR;

  if (dynamic_cast<
          DataObjects::MDEventWorkspace<DataObjects::MDLeanEvent<ND>, ND> *>(
          m_OutWSWrapper->pWorkspace().get()))
    return LEAN;
  return NONE;
}
} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_ */