// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_
#define MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_

#include  "MantidMDAlgorithms/ConvToMDEventsWS.h"

namespace Mantid {
// Forward declarations
namespace API {
class Progress;
}
namespace MDAlgorithms {

class ConvToMDEventsWSIndexing : public ConvToMDEventsWS {
  enum MD_EVENT_TYPE {
    LEAN,
    REGULAR,
    NONE
  };

  enum MD_BOX_TYPE {
    GRID,
    LEAF,
  };

  template <size_t ND>
  MD_EVENT_TYPE mdEventType();

  void appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc) override;

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

  template<typename EventType, size_t ND, template <size_t> class MDEventType>
  void appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc);

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
  static DataObjects::MDBoxBase<MDEventType<ND>, ND>*
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


  template<size_t ND, template <size_t> class MDEventType, typename EventIterator>
  static void distributeEvents(DataObjects::MDBoxBase<MDEventType<ND>, ND>* root,
                                                  const EventIterator begin,
                                                  const EventIterator end,
                                                  const typename MDEventType<ND>::MortonT lower,
                                                  const typename MDEventType<ND>::MortonT upper,
                                                  const MDSpaceBounds<ND>& space,
                                                  const size_t childBoxCount,
                                                  const size_t splitThreshold,
                                                  size_t maxDepth,
                                                  unsigned level,
                                                  const API::BoxController_sptr &bc);
};


template<typename EventType, size_t ND, template <size_t> class MDEventType>
std::vector<MDEventType<ND>> ConvToMDEventsWSIndexing::convertEvents() {
  size_t numEvents{0};
  for (size_t workspaceIndex = 0; workspaceIndex < m_NSpectra; ++workspaceIndex) {
    const Mantid::DataObjects::EventList &el = m_EventWS->getSpectrum(workspaceIndex);
    numEvents += el.getNumberEvents();
  }
  std::vector<MDEventType<ND>> mdEvents;
  mdEvents.reserve(numEvents);

#pragma omp parallel for
  for (size_t workspaceIndex = 0; workspaceIndex < m_NSpectra; ++workspaceIndex) {
    const auto& pws{m_OutWSWrapper->pWorkspace()};
    std::array<std::pair<coord_t, coord_t >, ND> bounds;
    for(size_t ax = 0; ax < ND; ++ ax) {
      bounds[ax] = std::make_pair(
          pws->getDimension(ax)->getMinimum(),
          pws->getDimension(ax)->getMaximum());
    }
    const Mantid::DataObjects::EventList &el = m_EventWS->getSpectrum(workspaceIndex);

    size_t numEvents = el.getNumberEvents();
    if (numEvents == 0)
      continue;

    // create local unit conversion class
    UnitsConversionHelper localUnitConv(m_UnitConversion);
    // create local QConverter
    MDTransf_sptr localQConverter(m_QConverter->clone());

    uint32_t detID = m_detID[workspaceIndex];
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
    distributeEvents(root, mdEvents.begin(), mdEvents.end(),
        mortonMin, mortonMax, space, bc->getNumSplit(),
        bc->getSplitThreshold(), bc->getMaxDepth() + 1, 1, bc);
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
    mdEvents[i].retrieveIndex(space);

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
void ConvToMDEventsWSIndexing::distributeEvents(DataObjects::MDBoxBase<MDEventType<ND>, ND>* root,
                                                const EventIterator begin,
                                                const EventIterator end,
                                                const typename MDEventType<ND>::MortonT lowerBound,
                                                const typename MDEventType<ND>::MortonT upperBound,
                                                const MDSpaceBounds<ND>& space,
                                                const size_t childBoxCount,
                                                const size_t splitThreshold,
                                                size_t maxDepth,
                                                unsigned level,
                                                const API::BoxController_sptr &bc) {
  using MDEvent = MDEventType<ND>;
  using MortonT = typename MDEvent::MortonT;
  using BoxBase = DataObjects::MDBoxBase<MDEvent, ND>;
  using Box = DataObjects::MDBox<MDEvent, ND>;
  using GridBox = DataObjects::MDGridBox<MDEvent, ND>;

  if (maxDepth-- == 1 || std::distance(begin, end) <= static_cast<int>(splitThreshold)) {
    return;
  }

  std::vector<API::IMDNode *> children;
  children.reserve(childBoxCount);

/* Determine the "width" of this box in Morton number */
  const MortonT thisBoxWidth = upperBound - lowerBound;

  /* Determine the "width" of the child boxes in Morton number */
  const MortonT childBoxWidth = thisBoxWidth / childBoxCount;

  auto eventIt = begin;

  std::vector<std::pair<EventIterator, EventIterator>> eventRanges;
  std::vector<std::pair<MortonT, MortonT>> mortonBounds;

  /* For each new child box */
  for (size_t i = 0; i < childBoxCount; ++i) {
    /* Lower child box bound is parent box lower bound plus for each previous
     * child box; box width plus offset by one (such that lower bound of box
     * i+1 is one grater than upper bound of box i) */
    const auto boxLower = lowerBound + ((childBoxWidth + 1) * i);

    /* Upper child box bound is lower plus child box width */
    const auto boxUpper = childBoxWidth + boxLower;

    const auto boxEventStart = eventIt;

    if (md_structure_ws::morton_contains<MortonT>(boxLower, boxUpper,eventIt->getIndex()))
      eventIt = std::upper_bound(boxEventStart, end, boxUpper,
          [](const MortonT& m, const typename std::iterator_traits<EventIterator>::value_type& event){
            return m < event.getIndex();
          });

    /* Add new child box. */
    /* As we are adding as we iterate over Morton numbers and parent events
     * child boxes are inserted in the correct sorted order. */
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents(ND);
    auto minCoord = MDEvent::indexToCoordinates(boxLower, space);
    auto maxCoord = MDEvent::indexToCoordinates(boxUpper, space);
    for(unsigned ax = 0; ax < ND; ++ax) {
      extents[ax].setExtents(minCoord[ax], maxCoord[ax]);
    }

    BoxBase* newBox;
    if(std::distance(boxEventStart, eventIt) <= static_cast<int>(splitThreshold) || maxDepth == 1) {
      for(auto it = boxEventStart; it < eventIt; ++it)
        it->retrieveCoordinates(space);
      bc->incBoxesCounter(level);
      newBox = new Box(bc.get(), level, extents, boxEventStart, eventIt);
    } else {
      bc->incGridBoxesCounter(level);
      newBox = new GridBox(bc.get(), level, extents, boxEventStart, eventIt);
    }

    children.emplace_back(newBox);
    eventRanges.emplace_back(std::make_pair(boxEventStart, eventIt));
    mortonBounds.emplace_back(std::make_pair(boxLower, boxUpper));
  }

  root->setChildren(children, 0, children.size());

  /* Distribute events within child boxes */
  /* See https://en.wikibooks.org/wiki/OpenMP/Tasks */
  /* The parallel pragma enables execution of the following block by all worker
   * threads. */
  ++level;
#pragma omp parallel for
  for (size_t i = 0; i < eventRanges.size(); i++) {
    BoxBase* childPtr = static_cast<BoxBase*>(children[i]);
    auto chBegin = eventRanges[i].first;
    auto chEnd = eventRanges[i].second;
    auto lower = mortonBounds[i].first;
    auto upper = mortonBounds[i].second;
    distributeEvents(childPtr, chBegin, chEnd, lower, upper,
                     space, childBoxCount, splitThreshold,
                     maxDepth, level, bc);
  }
}

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_ */