// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_
#define MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_

#include  "MantidMDAlgorithms/ConvToMDEventsWS.h"
#include <queue>
#include <mutex>
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
class ConvToMDEventsWSIndexing : public ConvToMDEventsWS, public DataObjects::EventAccessor {
  enum MD_EVENT_TYPE {
    LEAN,
    REGULAR,
    NONE
  };

  /**
   * Class to create the box structure of MDWorkspace. The algorithm:
   * The MASTER thread starting to build tree structure recursively,
   * if it finds the subtask to distribute N events N < threshold, the
   * it delegates this independent subtask to other tread, syncronisation
   * is implemented with queue and mutex.
   * @tparam ND :: number of Dimensions
   * @tparam MDEventType :: Type of created MDEvent [MDLeanEvent, MDEvent]
   * @tparam EventIterator :: Iterator of sorted collection storing the converted events
   */
  template<size_t ND, template <size_t> class MDEventType, typename EventIterator>
  class EventsDistributor {
  public:
    enum WORKER_TYPE {
      MASTER,
      SLAVE
    };
    /**
     * Structure to store the subtask of creating subtree from the
     * range of events
     */
    struct Task {
      DataObjects::MDBoxBase<MDEventType<ND>, ND>* root;
      const EventIterator begin;
      const EventIterator end;
      const typename MDEventType<ND>::MortonT lowerBound;
      const typename MDEventType<ND>::MortonT upperBound;
      const MDSpaceBounds<ND>& space;
      size_t maxDepth;
      unsigned level;
      const API::BoxController_sptr &bc;
    };
  public:
    EventsDistributor(const int& numWorkers, const size_t& threshold) :
    m_numWorkers(numWorkers), m_eventsThreshold(threshold), m_masterFinished{false} {};
    /**
     * Top level function to build tree structure
     * @param tsk :: top level task to start building the box structure
     */
    void distribute(Task& tsk) {
      if(m_numWorkers == 1)
        distributeEvents(tsk, SLAVE);
      else {
        std::vector<std::thread> workers;
        workers.emplace_back([this, &tsk]() {
          distributeEvents(tsk, MASTER);
          m_masterFinished = true;
          waitAndLaunchSlave();
        });
        for(auto i = 1; i < m_numWorkers; ++i)
          workers.emplace_back(&EventsDistributor::waitAndLaunchSlave, this);
        for(auto& worker: workers)
          worker.join();
      }
    }
  private:
    void distributeEvents(Task& tsk, const WORKER_TYPE& wtp);
    void pushTask(Task&& tsk) {
      std::lock_guard<std::mutex> g(m_mutex);
      m_tasks.emplace(tsk);
    }
    std::unique_ptr<Task> popTask() {
      std::lock_guard<std::mutex> g(m_mutex);
      if(m_tasks.empty())
        return std::unique_ptr<Task>(nullptr);
      else {
        std::unique_ptr<Task> task{new Task(m_tasks.front())};
        m_tasks.pop();
        return task;
      }
    }

    void waitAndLaunchSlave() {
      while(true) {
        auto pTsk = popTask();
        if(pTsk)
          distributeEvents(*pTsk.get(), SLAVE);
        else
          if(m_masterFinished) break;
      }
    }
  private:
    const int m_numWorkers;

    const size_t m_eventsThreshold;
    std::queue<Task> m_tasks;
    std::mutex m_mutex;
    std::atomic<bool> m_masterFinished;
  };

  //Interface function
  void appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) override;

private:
  template <size_t ND>
  MD_EVENT_TYPE mdEventType();

  // Wrapper to have the proper functions, for Nd in range 2 to maxDim
  template <size_t maxDim>
  void appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) {
    auto ndim = m_OutWSWrapper->nDimensions();
    if(ndim < 2)
      throw std::runtime_error("Can't convert to MD workspace with dims " + std::to_string(ndim) + "less than 2");
    if(ndim > maxDim)
      return;
    if(ndim == maxDim) {
      appendEvents<maxDim>(pProgress, bc);
      return;
    }
    else
      appendEventsFromInputWS<maxDim - 1>(pProgress, bc);
  }

  // Wrapper for ToF events of different types, number of dims, MD event type
  template<typename EventType, size_t ND, template <size_t> class MDEventType>
  void appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc);

  // Specialization for ToF events of different types
  template<size_t ND, template <size_t> class MDEventType>
  void appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc) {
    switch (m_EventWS->getSpectrum(0).getEventType()) {
    case Mantid::API::TOF:
      appendEvents<Mantid::Types::Event::TofEvent, ND, MDEventType>(pProgress, bc);
      break;
    case Mantid::API::WEIGHTED:
      appendEvents<Mantid::DataObjects::WeightedEvent, ND, MDEventType>(pProgress, bc);
      break;
    case Mantid::API::WEIGHTED_NOTIME:
      appendEvents<Mantid::DataObjects::WeightedEventNoTime, ND, MDEventType>(pProgress, bc);
      break;
    default:
      throw std::runtime_error("Events in event workspace had an unexpected data type!");
    }
  }

  // Specilization for MD event types
  template <size_t ND>
  void appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc) {
    switch(mdEventType<ND>()) {
    case LEAN:
      appendEvents<ND, DataObjects::MDLeanEvent>(pProgress, bc);
      break;
    case REGULAR:
      appendEvents<ND, DataObjects::MDEvent>(pProgress, bc);
      break;
    default:
      throw std::runtime_error("MD events in md event workspace had an unexpected data type!");
    }
  }




  template<size_t ND, template <size_t> class MDEventType>
  DataObjects::MDBoxBase<MDEventType<ND>, ND>*
  buildStructureFromSortedEvents(const API::BoxController_sptr &bc,
                                 std::vector<MDEventType<ND>> &mdEvents,
                                 const MDSpaceBounds<ND>& space,
                                 const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>& extents);

  template<typename EventType, size_t ND, template <size_t> class MDEventType>
  std::vector<MDEventType<ND>> convertEvents();

  template <size_t ND, template <size_t> class MDEventType>
  struct MDEventMaker {
    static MDEventType<ND> makeMDEvent(const double &sig,
                                       const double &err,
                                       const uint16_t &run_index,
                                       const uint32_t &det_id,
                                       coord_t *coord) {
      return MDEventType<ND>(sig, err, run_index, det_id, coord);
    }
  };

};

/*-------------------------------definitions-------------------------------------*/


template<typename EventType, size_t ND, template <size_t> class MDEventType>
std::vector<MDEventType<ND>> ConvToMDEventsWSIndexing::convertEvents() {
  size_t numEvents{0};
  for (size_t workspaceIndex = 0; workspaceIndex < m_NSpectra; ++workspaceIndex) {
    const Mantid::DataObjects::EventList &el = m_EventWS->getSpectrum(workspaceIndex);
    numEvents += el.getNumberEvents();
  }
  std::vector<MDEventType<ND>> mdEvents;
  mdEvents.reserve(numEvents);

  const auto& pws{m_OutWSWrapper->pWorkspace()};
  std::array<std::pair<coord_t, coord_t >, ND> bounds;
  for(size_t ax = 0; ax < ND; ++ ax) {
    bounds[ax] = std::make_pair(
        pws->getDimension(ax)->getMinimum(),
        pws->getDimension(ax)->getMaximum());
  }

#pragma omp parallel for
  for (size_t workspaceIndex = 0; workspaceIndex < m_NSpectra; ++workspaceIndex) {
    const Mantid::DataObjects::EventList &el = m_EventWS->getSpectrum(workspaceIndex);

    size_t numEvents = el.getNumberEvents();
    if (numEvents == 0)
      continue;

    // create local unit conversion class
    UnitsConversionHelper localUnitConv(m_UnitConversion);
    // create local QConverter
    MDTransf_sptr localQConverter(m_QConverter->clone());

    int32_t detID = m_detID[workspaceIndex];
    uint16_t runIndexLoc = m_RunIndex;

    std::vector<coord_t> locCoord(m_Coord);
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
    for (auto it = events.cbegin(); it != events.cend(); it++) {
      double val = localUnitConv.convertUnits(it->tof());
      double signal = it->weight();
      double errorSq = it->errorSquared();

      if (!localQConverter->calcMatrixCoord(val, locCoord, signal, errorSq))
        continue; // skip ND outside the range

      mdEventsForSpectrum.emplace_back(MDEventMaker<ND, MDEventType>::
                                       makeMDEvent(signal, errorSq, runIndexLoc, detID, &locCoord[0]));


      // Filter events before adding to the ndEvents vector to add in workspace
      // The bounds of the resulting WS have to be already defined
      bool isInOutWSBox = true;
      for(size_t ax = 0; ax < ND; ++ ax) {
        const coord_t& coord{mdEventsForSpectrum.back().getCenter(ax)};
        if (coord < bounds[ax].first || coord > bounds[ax].second)
          isInOutWSBox = false;
      }

      if(!isInOutWSBox)
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


template<size_t ND, template <size_t> class MDEventType>
DataObjects::MDBoxBase<MDEventType<ND>, ND>*
ConvToMDEventsWSIndexing::buildStructureFromSortedEvents(const API::BoxController_sptr &bc,
                                                         std::vector<MDEventType<ND>> &mdEvents,
                                                         const MDSpaceBounds<ND>& space,
                                                         const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>>& extents) {
  using MDEvent = MDEventType<ND>;
  using IntT = typename MDEvent::IntT;
  using MortonT = typename MDEvent::MortonT;
  using DistributorType = EventsDistributor<ND, MDEventType, typename std::vector<MDEventType<ND>>::iterator>;

  if(mdEvents.size() <= bc->getSplitThreshold()) {
    bc->incBoxesCounter(0);
    return new DataObjects::MDBox<MDEvent, ND>(bc.get(), 0, extents, mdEvents.begin(), mdEvents.end());
  } else {
    auto root = new DataObjects::MDGridBox<MDEvent, ND>(bc.get(), 0, extents, mdEvents.begin(), mdEvents.end());
    auto mortonMin =
        md_structure_ws::calculateDefaultBound<ND, IntT, MortonT>(
            std::numeric_limits<IntT>::min());
    auto mortonMax =
        md_structure_ws::calculateDefaultBound<ND, IntT, MortonT>(
            std::numeric_limits<IntT>::max());
    typename DistributorType::Task tsk{root, mdEvents.begin(), mdEvents.end(),
        mortonMin, mortonMax, space, bc->getMaxDepth() + 1, 1, bc};
    int nThreads = (this->m_NumThreads == 0); // 1 thread if 0
    if(!nThreads)
      nThreads = this->m_NumThreads < 0 ? PARALLEL_GET_MAX_THREADS : this->m_NumThreads;
    DistributorType distributor(nThreads, mdEvents.size() / nThreads/ 10);
    distributor.distribute(tsk);
    return root;
  }
}


template<typename EventType, size_t ND, template <size_t> class MDEventType>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  bc->clearBoxesCounter(1);
  bc->clearGridBoxesCounter(0);
  pProgress->resetNumSteps(4, 0, 1);

  std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
  start = std::chrono::high_resolution_clock::now();

  std::vector<MDEventType<ND>> mdEvents = convertEvents<EventType, ND, MDEventType>();
  MDSpaceBounds<ND> space;
  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents;
  const auto& pws{m_OutWSWrapper->pWorkspace()};
  for(size_t ax = 0; ax < ND; ++ ax) {
    space(ax, 0) = pws->getDimension(ax)->getMinimum();
    space(ax, 1) = pws->getDimension(ax)->getMaximum();
    extents.emplace_back();
    extents.rbegin()->setExtents(pws->getDimension(ax)->getMinimum(), pws->getDimension(ax)->getMaximum());
  }

  pProgress->report(0);

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Convert events: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();


#pragma omp parallel for
  for(size_t i = 0; i < mdEvents.size(); ++i)
    MDEventType<ND>::template AccessFor<ConvToMDEventsWSIndexing>::retrieveIndex(mdEvents[i], space);

  pProgress->report(1);

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Retrieve morton: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();



  tbb::parallel_sort(mdEvents.begin(), mdEvents.end(), [] (const MDEventType<ND>& a, const MDEventType<ND>& b) {
    return a.getIndex() < b.getIndex();
  });

  pProgress->report(2);


  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Sort: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();

  auto root = buildStructureFromSortedEvents<ND, MDEventType>(bc, mdEvents, space, extents);
  m_OutWSWrapper->pWorkspace()->setBox(root);
  pProgress->report(3);

  end = std::chrono::high_resolution_clock::now();
  std::cerr <<  "Build native boxes: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();

  root->calculateGridCaches();

  end = std::chrono::high_resolution_clock::now();
  std::cerr <<  "Calculate caches: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
}



template <size_t ND>
struct ConvToMDEventsWSIndexing::MDEventMaker<ND, Mantid::DataObjects::MDLeanEvent> {
  static Mantid::DataObjects::MDLeanEvent<ND> makeMDEvent(const double &sig,
                                                          const double &err,
                                                          const uint16_t,
                                                          const uint32_t,
                                                          coord_t *coord) {
    return Mantid::DataObjects::MDLeanEvent<ND>(sig, err, coord);
  }
};

template <size_t ND>
ConvToMDEventsWSIndexing::MD_EVENT_TYPE ConvToMDEventsWSIndexing::mdEventType() {
  if(
      dynamic_cast<
          DataObjects::MDEventWorkspace<DataObjects::MDEvent<ND>, ND> *>(
          m_OutWSWrapper->pWorkspace().get())
      )
    return REGULAR;

  if(
      dynamic_cast<
          DataObjects::MDEventWorkspace<DataObjects::MDLeanEvent<ND>, ND> *>(
          m_OutWSWrapper->pWorkspace().get())
      )
    return LEAN;
  return NONE;
}


template<size_t ND, template <size_t> class MDEventType, typename EventIterator>
void ConvToMDEventsWSIndexing::EventsDistributor<ND, MDEventType, EventIterator>::
    distributeEvents(Task& tsk, const WORKER_TYPE& wtp) {
  using MDEvent = MDEventType<ND>;
  using MortonT = typename MDEvent::MortonT;
  using BoxBase = DataObjects::MDBoxBase<MDEvent, ND>;
  using Box = DataObjects::MDBox<MDEvent, ND>;
  using GridBox = DataObjects::MDGridBox<MDEvent, ND>;


  const size_t childBoxCount = tsk.bc->getNumSplit();
  const size_t splitThreshold = tsk.bc->getSplitThreshold();

  if (tsk.maxDepth-- == 1 || std::distance(tsk.begin, tsk.end) <= static_cast<int>(splitThreshold)) {
    return;
  }

/* Determine the "width" of this box in Morton number */
  const MortonT thisBoxWidth = tsk.upperBound - tsk.lowerBound;

  /* Determine the "width" of the child boxes in Morton number */
  const MortonT childBoxWidth = thisBoxWidth / childBoxCount;

  auto eventIt = tsk.begin;

  struct RecursionHelper {
    std::pair<EventIterator, EventIterator> eventRange;
    std::pair<MortonT, MortonT> mortonBounds;
    BoxBase* box;
  };
  std::vector<RecursionHelper> children;
  children.reserve(childBoxCount);

  /* For each new child box */
  for (size_t i = 0; i < childBoxCount; ++i) {
    /* Lower child box bound is parent box lower bound plus for each previous
     * child box; box width plus offset by one (such that lower bound of box
     * i+1 is one grater than upper bound of box i) */
    const auto boxLower = tsk.lowerBound + ((childBoxWidth + 1) * i);

    /* Upper child box bound is lower plus child box width */
    const auto boxUpper = childBoxWidth + boxLower;

    const auto boxEventStart = eventIt;

    if (md_structure_ws::morton_contains<MortonT>(boxLower, boxUpper,eventIt->getIndex()))
      eventIt = std::upper_bound(boxEventStart, tsk.end, boxUpper,
          [](const MortonT& m, const typename std::iterator_traits<EventIterator>::value_type& event){
            return m < event.getIndex();
          });

    /* Add new child box. */
    /* As we are adding as we iterate over Morton numbers and parent events
     * child boxes are inserted in the correct sorted order. */
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents(ND);
    auto minCoord = MDEvent::indexToCoordinates(boxLower, tsk.space);
    auto maxCoord = MDEvent::indexToCoordinates(boxUpper, tsk.space);
    for(unsigned ax = 0; ax < ND; ++ax) {
      extents[ax].setExtents(minCoord[ax], maxCoord[ax]);
    }

    BoxBase* newBox;
    if(std::distance(boxEventStart, eventIt) <= static_cast<int>(splitThreshold) || tsk.maxDepth == 1) {
      for(auto it = boxEventStart; it < eventIt; ++it)
        MDEventType<ND>::template AccessFor<ConvToMDEventsWSIndexing>::retrieveCoordinates(*it, tsk.space);
      tsk.bc->incBoxesCounter(tsk.level);
      newBox = new Box(tsk.bc.get(), tsk.level, extents, boxEventStart, eventIt);
    } else {
      tsk.bc->incGridBoxesCounter(tsk.level);
      newBox = new GridBox(tsk.bc.get(), tsk.level, extents);
    }

    children.emplace_back(RecursionHelper{{boxEventStart, eventIt}, {boxLower, boxUpper}, newBox});
  }
  //sorting is needed due to fast finding the proper box for given coordinate,
  //during drawing
  std::sort(children.begin(), children.end(), [](RecursionHelper& a, RecursionHelper& b) {
    unsigned i = ND; while(i-->0) {
    const auto& ac = a.box->getExtents(i).getMin();
    const auto& bc = b.box->getExtents(i).getMin();
      if (ac < bc) return true;
      if (ac > bc) return false;
    }
    return true;
  });
  std::vector<API::IMDNode *> boxes;
  boxes.reserve(childBoxCount);
  for(auto& ch: children)
    boxes.emplace_back(ch.box);
  tsk.root->setChildren(boxes, 0, boxes.size());

  ++tsk.level;
  for (auto& ch: children) {
    Task task {ch.box, ch.eventRange.first, ch.eventRange.second,
                              ch.mortonBounds.first, ch.mortonBounds.second,
                              tsk.space, tsk.maxDepth, tsk.level, tsk.bc};
    if(wtp == MASTER && (size_t)std::distance(task.begin, task.end) < m_eventsThreshold)
      pushTask(std::move(task));
    else
      distributeEvents(task, wtp);
  }
}

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_ */