// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_
#define MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_

#include  "MantidMDAlgorithms/ConvToMDEventsWS.h"
#include "queue"

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

  template<size_t ND, template <size_t> class MDEventType>
  using BoxStructureType = md_structure_ws::MDBox<ND, typename MDEventType<ND>::IntT,
                                                  typename MDEventType<ND>::MortonT, MDEventType>;

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
  struct StructureToNativeBox {
    unsigned level;
    std::reference_wrapper<const ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>> sBox;
    std::reference_wrapper<DataObjects::MDGridBox<MDEventType<ND>, ND>> nBox;

    friend bool operator<(const StructureToNativeBox& a, const StructureToNativeBox& b) {return a.level < b.level; }
  };

  template<size_t ND, template <size_t> class MDEventType>
  void convertToNativeBoxStructureRecursive(const ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>& sBoxCur,
                                            DataObjects::MDGridBox<MDEventType<ND>, ND>& nBoxCur,
                                            const MDSpaceBounds<ND>& space,
                                            const API::BoxController_sptr &bc,
                                            unsigned level, unsigned partialLevel,
                                            std::vector<StructureToNativeBox<ND, MDEventType>>& boxMapBuffer);

  template<size_t ND, template <size_t> class MDEventType>
  DataObjects::MDBoxBase<MDEventType<ND>, ND>*
  convertToNativeBoxStructure(const BoxStructureType<ND, MDEventType>& mdBox,
                              const MDSpaceBounds<ND>& space,
                              const API::BoxController_sptr &bc);

  template<size_t ND, template <size_t> class MDEventType>
  std::unique_ptr<ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>>
  buildStructureFromSortedEvents(const API::BoxController_sptr &bc,
                                 const std::vector<MDEventType<ND>> &mdEvents);

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

  template <size_t ND, template <size_t> class MDEventType>
  static DataObjects::MDBoxBase<MDEventType<ND>, ND>* makeMDBox(BoxStructureType<ND, MDEventType> sBox,
                                                                const MD_BOX_TYPE& type,
                                                                const MDSpaceBounds<ND>& space,
                                                                const API::BoxController_sptr &bc,
                                                                const unsigned& level);
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
        if (
            coord < pws->getDimension(ax)->getMinimum() ||
                coord >pws->getDimension(ax)->getMaximum()
            )
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
void ConvToMDEventsWSIndexing::convertToNativeBoxStructureRecursive(
    const ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>& sBoxCur,
    DataObjects::MDGridBox<MDEventType<ND>, ND>& nBoxCur,
    const MDSpaceBounds<ND>& space,
    const API::BoxController_sptr &bc,
    unsigned level, unsigned partialLevel,
    std::vector<StructureToNativeBox<ND, MDEventType>>& boxMapBuffer) {
  std::vector<StructureToNativeBox<ND, MDEventType>> boxMapBufferLocal;
  const auto& sChildren = sBoxCur.children();
  std::vector<API::IMDNode *> children;
  for(unsigned i = 0; i < sChildren.size(); ++i) {
    children.reserve(sChildren.size());
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents(ND);
    if(sChildren[i].isLeaf()) {
      bc->incBoxesCounter(level);
      children.emplace_back(makeMDBox<ND, MDEventType>(sChildren[i], ConvToMDEventsWSIndexing::LEAF, space, bc, level));
    } else {
      bc->incGridBoxesCounter(level);
      auto gridBoxPtr =
          static_cast<DataObjects::MDGridBox<MDEventType<ND>, ND>*>
          (makeMDBox<ND, MDEventType>(sChildren[i], ConvToMDEventsWSIndexing::GRID, space, bc, level));
      children.emplace_back(gridBoxPtr);
      boxMapBufferLocal.emplace_back(StructureToNativeBox<ND, MDEventType>{level, std::cref(sChildren[i]), std::ref(*gridBoxPtr)});
    }
  }
  nBoxCur.setChildren(children, 0, children.size());

  ++level;
  if(level < partialLevel) {
    for (auto &bm: boxMapBufferLocal) {
      ConvToMDEventsWSIndexing::
      convertToNativeBoxStructureRecursive<ND, MDEventType>(bm.sBox, bm.nBox, space, bc, level, partialLevel, boxMapBuffer);
    }
  } else {
    boxMapBuffer.insert(boxMapBuffer.end(), boxMapBufferLocal.begin(), boxMapBufferLocal.end());
  }
}

template<size_t ND, template <size_t> class MDEventType>
DataObjects::MDBoxBase<MDEventType<ND>, ND>* ConvToMDEventsWSIndexing::
convertToNativeBoxStructure(const ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType> & mdBox,
                            const MDSpaceBounds<ND>& space, const API::BoxController_sptr &bc) {
  if(mdBox.isLeaf()) {
    return makeMDBox<ND, MDEventType>(mdBox, ConvToMDEventsWSIndexing::LEAF, space, bc, 0);
  }
  else {
    DataObjects::MDGridBox<MDEventType<ND>, ND>*
        res(
        static_cast<DataObjects::MDGridBox<MDEventType<ND>, ND> *>
        (makeMDBox<ND, MDEventType>(mdBox, ConvToMDEventsWSIndexing::GRID, space, bc, 0))
    );
    std::vector<ConvToMDEventsWSIndexing::StructureToNativeBox<ND, MDEventType>> mapBoxBuf, dummy;
    if(m_NumThreads == 1) {
      convertToNativeBoxStructureRecursive<ND, MDEventType>(mdBox, *res, space, bc, 1, bc->getMaxDepth() + 1, mapBoxBuf);
      return res;
    } else {
      //this has the same behavior as in ConvertToMdEventsWS class
      unsigned nThreads = m_NumThreads > 0 ? m_NumThreads : Kernel::ThreadPool::getNumPhysicalCores();
      // How many preliminary steps we need to have enough jobs for parallelizind ~10*nThreads
      unsigned toLevel = log(10*nThreads)/(ND*log(bc->getSplitInto(0))) + 1;
      convertToNativeBoxStructureRecursive<ND, MDEventType>(mdBox, *res, space, bc, 1, toLevel, mapBoxBuf);
      std::cerr << "toLevel = " << toLevel << "  Jobs count: " << mapBoxBuf.size() << "\n"; //TODO tmp
#pragma omp parallel for num_threads(nThreads)
      for(unsigned i = 0; i < mapBoxBuf.size(); ++i)
        convertToNativeBoxStructureRecursive<ND, MDEventType>(mapBoxBuf[i].sBox, mapBoxBuf[i].nBox, space, bc, toLevel, bc->getMaxDepth() + 1, dummy);
      return res;
    }
  }
}

template<size_t ND, template <size_t> class MDEventType>
std::unique_ptr<ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>>
ConvToMDEventsWSIndexing::buildStructureFromSortedEvents(const API::BoxController_sptr &bc,
                                                         const std::vector<MDEventType<ND>> &mdEvents) {
  auto rootMdBox =
      std::make_unique<ConvToMDEventsWSIndexing::BoxStructureType<ND, MDEventType>>(mdEvents.cbegin(), mdEvents.cend());
  rootMdBox->distributeEvents(bc->getSplitThreshold(), bc->getMaxDepth() + 1);

  auto leafs = rootMdBox->leafs();
  size_t maxEvents = 0;
  size_t maxDepth = 0;
  for(auto& leaf: leafs) {
    if(leaf.level > maxDepth)
      maxDepth = leaf.level;
    if(leaf.box.eventCount() > maxEvents)
      maxEvents = leaf.box.eventCount();
  }
  return rootMdBox;
}



template<typename EventType, size_t ND, template <size_t> class MDEventType>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  pProgress->resetNumSteps(4, 0, 1);

  std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
  start = std::chrono::high_resolution_clock::now();


  std::vector<MDEventType<ND>> mdEvents = convertEvents<EventType, ND, MDEventType>();
  MDSpaceBounds<ND> space;
  const auto& pws{m_OutWSWrapper->pWorkspace()};
  for(size_t ax = 0; ax < ND; ++ ax) {
    space(ax, 0) = pws->getDimension(ax)->getMinimum();
    space(ax, 1) = pws->getDimension(ax)->getMaximum();
  }

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Convert events: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();


#pragma omp parallel for
  for(size_t i = 0; i < mdEvents.size(); ++i)
    mdEvents[i].retrieveIndex(space);

  pProgress->report(0);

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Retrieve morton: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();



  tbb::parallel_sort(mdEvents.begin(), mdEvents.end(), [] (const MDEventType<ND>& a, const MDEventType<ND>& b) {
    return a.getIndex() < b.getIndex();
  });

  pProgress->report(1);


  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Sort: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();


  auto rootMdBox = buildStructureFromSortedEvents<ND, MDEventType>(bc, mdEvents);

  pProgress->report(2);

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Build boxes: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();


#pragma omp parallel for
  for(size_t i = 0; i < mdEvents.size(); ++i)
    mdEvents[i].retrieveCoordinates(space);


  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Retrieve cordinates: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
  start = std::chrono::high_resolution_clock::now();


  m_OutWSWrapper->pWorkspace()->setBox(convertToNativeBoxStructure<ND, MDEventType>(*(rootMdBox.get()), space, bc));

  pProgress->report(3);

  end = std::chrono::high_resolution_clock::now();
  std::cerr << "Convert boxes: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "\n";
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

template <size_t ND, template <size_t> class MDEventType>
DataObjects::MDBoxBase<MDEventType<ND>, ND>*
ConvToMDEventsWSIndexing::makeMDBox(BoxStructureType<ND, MDEventType> sbox, const MD_BOX_TYPE& type,
                                    const MDSpaceBounds<ND>& space, const API::BoxController_sptr &bc,
                                    const unsigned& level) {
  std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> extents(ND);
  auto minCoord = MDEventType<ND>::indexToCoordinates(sbox.min(), space);
  auto maxCoord = MDEventType<ND>::indexToCoordinates(sbox.max(), space);
  for(unsigned i = 0; i < ND; ++i) {
    extents[i].setExtents(minCoord[i], maxCoord[i]);
  }
  switch(type) {
  case LEAF: {
    return new DataObjects::MDBox<MDEventType<ND>, ND>(bc.get(), level, extents, sbox.eventBegin(), sbox.eventEnd());
  }
  case GRID:
    return new DataObjects::MDGridBox<MDEventType<ND>, ND>(bc.get(), level, extents, sbox.eventBegin(), sbox.eventEnd());
  default:
    throw std::logic_error("Wrong MD box type detected.");
  }

}

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_CONVTOMDEVENTSWSINDEXING_H_ */