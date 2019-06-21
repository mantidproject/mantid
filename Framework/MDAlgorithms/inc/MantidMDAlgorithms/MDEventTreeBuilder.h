// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_MDEVENTTREEBUILDER_H_
#define MANTID_MDALGORITHMS_MDEVENTTREEBUILDER_H_

#include <queue>
#include <tbb/parallel_sort.h>
#include <tbb/task_scheduler_init.h>
#include <thread>

namespace Mantid {
namespace MDAlgorithms {

/**
 * Class to create the box structure of MDWorkspace. The algorithm:
 * The MASTER thread starting to build tree structure recursively,
 * if it finds the subtask to distribute N events N < threshold, the
 * it delegates this independent subtask to other tread, syncronisation
 * is implemented with queue and mutex.
 * @tparam ND :: number of Dimensions
 * @tparam MDEventType :: Type of created MDEvent [MDLeanEvent, MDEvent]
 * @tparam EventIterator :: Iterator of sorted collection storing the converted
 * events
 */
template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
class MDEventTreeBuilder {
  using MDEvent = MDEventType<ND>;
  using IntT = typename MDEvent::IntT;
  using MortonT = typename MDEvent::MortonT;
  using BoxBase = DataObjects::MDBoxBase<MDEvent, ND>;
  using Box = DataObjects::MDBox<MDEvent, ND>;
  using GridBox = DataObjects::MDGridBox<MDEvent, ND>;
  using EventDistributor = MDEventTreeBuilder<ND, MDEventType, EventIterator>;

public:
  using EventAccessType = DataObjects::EventAccessor;
  using IndexCoordinateSwitcher =
      typename MDEvent::template AccessFor<EventDistributor>;
  enum WORKER_TYPE { MASTER, SLAVE };
  /**
   * Structure to store the subtask of creating subtree from the
   * range of events
   */
  struct Task {
    BoxBase *root;
    const EventIterator begin;
    const EventIterator end;
    const typename MDEventType<ND>::MortonT lowerBound;
    const typename MDEventType<ND>::MortonT upperBound;
    size_t maxDepth;
    unsigned level;
  };

public:
  MDEventTreeBuilder(const int numWorkers, const size_t threshold,
                     const API::BoxController_sptr &bc,
                     const morton_index::MDSpaceBounds<ND> &space);
  struct TreeWithIndexError {
    BoxBase *root;
    morton_index::MDCoordinate<ND> err;
  };
  /**
   *
   * @param mdEvents :: events to distribute around the tree
   * @return :: pointer to the root node and error
   */
  TreeWithIndexError distribute(std::vector<MDEventType<ND>> &mdEvents);

private:
  morton_index::MDCoordinate<ND>
  convertToIndex(std::vector<MDEventType<ND>> &mdEvents,
                 const morton_index::MDSpaceBounds<ND> &space);
  void sortEvents(std::vector<MDEventType<ND>> &mdEvents);
  BoxBase *doDistributeEvents(std::vector<MDEventType<ND>> &mdEvents);
  void distributeEvents(Task &tsk, const WORKER_TYPE &wtp);
  void pushTask(Task &&tsk);
  std::unique_ptr<Task> popTask();
  void waitAndLaunchSlave();

private:
  const int m_numWorkers;
  const size_t m_eventsThreshold;
  std::queue<Task> m_tasks;
  std::mutex m_mutex;
  std::atomic<bool> m_masterFinished;

  const morton_index::MDSpaceBounds<ND> &m_space;
  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> m_extents;
  const API::BoxController_sptr &m_bc;

  const MortonT m_mortonMin;
  const MortonT m_mortonMax;
};

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
MDEventTreeBuilder<ND, MDEventType, EventIterator>::MDEventTreeBuilder(
    const int numWorkers, const size_t threshold,
    const API::BoxController_sptr &bc,
    const morton_index::MDSpaceBounds<ND> &space)
    : m_numWorkers(numWorkers), m_eventsThreshold(threshold),
      m_masterFinished{false}, m_space{space}, m_bc{bc},
      m_mortonMin{morton_index::calculateDefaultBound<ND, IntT, MortonT>(
          std::numeric_limits<IntT>::min())},
      m_mortonMax{morton_index::calculateDefaultBound<ND, IntT, MortonT>(
          std::numeric_limits<IntT>::max())} {
  for (size_t ax = 0; ax < ND; ++ax) {
    m_extents.emplace_back();
    m_extents.back().setExtents(space(ax, 0), space(ax, 1));
  }
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
typename MDEventTreeBuilder<ND, MDEventType, EventIterator>::TreeWithIndexError
MDEventTreeBuilder<ND, MDEventType, EventIterator>::distribute(
    std::vector<MDEvent> &mdEvents) {
  auto err = convertToIndex(mdEvents, m_space);
  sortEvents(mdEvents);
  auto root = doDistributeEvents(mdEvents);
  return {root, err};
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
DataObjects::MDBoxBase<MDEventType<ND>, ND> *
MDEventTreeBuilder<ND, MDEventType, EventIterator>::doDistributeEvents(
    std::vector<MDEventType<ND>> &mdEvents) {
  if (mdEvents.size() <= m_bc->getSplitThreshold()) {
    m_bc->incBoxesCounter(0);
    return new DataObjects::MDBox<MDEvent, ND>(
        m_bc.get(), 0, m_extents, mdEvents.begin(), mdEvents.end());
  } else {
    auto root =
        new DataObjects::MDGridBox<MDEvent, ND>(m_bc.get(), 0, m_extents);
    Task tsk{root,
             mdEvents.begin(),
             mdEvents.end(),
             m_mortonMin,
             m_mortonMax,
             m_bc->getMaxDepth() + 1,
             1};

    if (m_numWorkers == 1)
      distributeEvents(tsk, SLAVE);
    else {
      std::vector<std::thread> workers;
      workers.emplace_back([this, &tsk]() {
        distributeEvents(tsk, MASTER);
        m_masterFinished = true;
        waitAndLaunchSlave();
      });
      for (auto i = 1; i < m_numWorkers; ++i)
        workers.emplace_back(&MDEventTreeBuilder::waitAndLaunchSlave, this);
      for (auto &worker : workers)
        worker.join();
    }
    return root;
  }
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
morton_index::MDCoordinate<ND>
MDEventTreeBuilder<ND, MDEventType, EventIterator>::convertToIndex(
    std::vector<MDEventType<ND>> &mdEvents,
    const morton_index::MDSpaceBounds<ND> &space) {
  std::vector<morton_index::MDCoordinate<ND>> perThread(
      m_numWorkers, morton_index::MDCoordinate<ND>(0));
#pragma omp parallel for num_threads(m_numWorkers)
  for (int64_t i = 0; i < static_cast<int64_t>(mdEvents.size()); ++i) {
    morton_index::MDCoordinate<ND> oldCoord{mdEvents[i].getCenter()};
    IndexCoordinateSwitcher::convertToIndex(mdEvents[i], space);
    morton_index::MDCoordinate<ND> newCoord =
        morton_index::indexToCoordinates<ND, IntT, MortonT>(
            IndexCoordinateSwitcher::getIndex(mdEvents[i]), space);
    newCoord -= oldCoord;
    morton_index::MDCoordinate<ND> &threadErr{
        perThread[PARALLEL_THREAD_NUMBER]};
    for (size_t d = 0; d < ND; ++d)
      threadErr[d] = std::max(threadErr[d], std::abs(newCoord[d]));
  }
  morton_index::MDCoordinate<ND> maxErr(0);
  for (const auto &err : perThread)
    for (size_t d = 0; d < ND; ++d)
      maxErr[d] = std::max(maxErr[d], err[d]);
  return maxErr;
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
void MDEventTreeBuilder<ND, MDEventType, EventIterator>::sortEvents(
    std::vector<MDEventType<ND>> &mdEvents) {
  tbb::task_scheduler_init init{m_numWorkers};
  tbb::parallel_sort(mdEvents.begin(), mdEvents.end(),
                     [](const MDEventType<ND> &a, const MDEventType<ND> &b) {
                       return IndexCoordinateSwitcher::getIndex(a) <
                              IndexCoordinateSwitcher::getIndex(b);
                     });
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
void MDEventTreeBuilder<ND, MDEventType, EventIterator>::pushTask(
    MDEventTreeBuilder<ND, MDEventType, EventIterator>::Task &&tsk) {
  std::lock_guard<std::mutex> g(m_mutex);
  m_tasks.emplace(tsk);
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
std::unique_ptr<
    typename MDEventTreeBuilder<ND, MDEventType, EventIterator>::Task>
MDEventTreeBuilder<ND, MDEventType, EventIterator>::popTask() {
  std::lock_guard<std::mutex> g(m_mutex);
  if (m_tasks.empty())
    return {nullptr};
  else {
    auto task = std::make_unique<Task>(m_tasks.front());
    m_tasks.pop();
    return task;
  }
}

template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
void MDEventTreeBuilder<ND, MDEventType, EventIterator>::waitAndLaunchSlave() {
  while (true) {
    auto pTsk = popTask();
    if (pTsk)
      distributeEvents(*pTsk.get(), SLAVE);
    else if (m_masterFinished)
      break;
    else
      std::this_thread::sleep_for(std::chrono::nanoseconds(100));
  }
}

/**
 * Does actual work on creating tasks in MASTER mode and
 * executing tasks in SLAVE mode
 */
template <size_t ND, template <size_t> class MDEventType,
          typename EventIterator>
void MDEventTreeBuilder<ND, MDEventType, EventIterator>::distributeEvents(
    Task &tsk, const WORKER_TYPE &wtp) {
  const size_t childBoxCount = m_bc->getNumSplit();
  const size_t splitThreshold = m_bc->getSplitThreshold();

  if (tsk.maxDepth-- == 1 ||
      std::distance(tsk.begin, tsk.end) <= static_cast<int>(splitThreshold)) {
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
    BoxBase *box;
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

    if (eventIt < tsk.end) {
      if (morton_index::morton_contains<MortonT>(
              boxLower, boxUpper, IndexCoordinateSwitcher::getIndex(*eventIt)))
        eventIt = std::upper_bound(
            boxEventStart, tsk.end, boxUpper,
            [](const MortonT &m,
               const typename std::iterator_traits<EventIterator>::value_type
                   &event) {
              return m < IndexCoordinateSwitcher::getIndex(event);
            });
    }

    /* Add new child box. */
    /* As we are adding as we iterate over Morton numbers and parent events
     * child boxes are inserted in the correct sorted order. */
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents(ND);
    auto minCoord =
        morton_index::indexToCoordinates<ND, IntT, MortonT>(boxLower, m_space);
    auto maxCoord =
        morton_index::indexToCoordinates<ND, IntT, MortonT>(boxUpper, m_space);
    for (unsigned ax = 0; ax < ND; ++ax) {
      extents[ax].setExtents(minCoord[ax], maxCoord[ax]);
    }

    BoxBase *newBox;
    if (std::distance(boxEventStart, eventIt) <=
            static_cast<int64_t>(splitThreshold) ||
        tsk.maxDepth == 1) {
      for (auto it = boxEventStart; it < eventIt; ++it)
        IndexCoordinateSwitcher::convertToCoordinates(*it, m_space);
      m_bc->incBoxesCounter(tsk.level);
      newBox = new Box(m_bc.get(), tsk.level, extents, boxEventStart, eventIt);
    } else {
      m_bc->incGridBoxesCounter(tsk.level);
      newBox = new GridBox(m_bc.get(), tsk.level, extents);
    }

    children.emplace_back(RecursionHelper{
        {boxEventStart, eventIt}, {boxLower, boxUpper}, newBox});
  }
  // sorting is needed due to fast finding the proper box for given coordinate,
  // during drawing, for splitInto != 2 Z-curve gives wrong order
  std::sort(children.begin(), children.end(),
            [](RecursionHelper &a, RecursionHelper &b) {
              unsigned i = ND;
              while (i-- > 0) {
                const auto &ac = a.box->getExtents(i).getMin();
                const auto &bc = b.box->getExtents(i).getMin();
                if (ac < bc)
                  return true;
                if (ac > bc)
                  return false;
              }
              return true;
            });
  std::vector<API::IMDNode *> boxes;
  boxes.reserve(childBoxCount);
  for (auto &ch : children)
    boxes.emplace_back(ch.box);
  tsk.root->setChildren(boxes, 0, boxes.size());

  ++tsk.level;
  for (auto &ch : children) {
    Task newTask{ch.box,
                 ch.eventRange.first,
                 ch.eventRange.second,
                 ch.mortonBounds.first,
                 ch.mortonBounds.second,
                 tsk.maxDepth,
                 tsk.level};
    if (wtp == MASTER &&
        (size_t)std::distance(newTask.begin, newTask.end) < m_eventsThreshold)
      pushTask(std::move(newTask));
    else
      distributeEvents(newTask, wtp);
  }
}

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_MDEVENTTREEBUILDER_H_ */