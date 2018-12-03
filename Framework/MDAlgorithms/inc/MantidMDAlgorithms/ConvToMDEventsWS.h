// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CONV_TOMD_EVENTSWS_H
#define MANTID_MDALGORITHMS_CONV_TOMD_EVENTSWS_H
//
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/MDEvent.h"
#include "MantidDataObjects/MDBoxBase.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidMDAlgorithms/ConvToMDBase.h"
#include "MantidMDAlgorithms/MDEventWSWrapper.h"
#include "MantidMDAlgorithms/MDTransfFactory.h"

#include <vector>
#include <tbb/parallel_sort.h>

namespace Mantid {
// Forward declarations
namespace API {
class Progress;
}
namespace MDAlgorithms {
/** The class specializes ConvToDataObjectsBase for the case when the conversion
  occurs from Events WS to the MD events WS
  *
  *
  * See http://www.mantidproject.org/Writing_custom_ConvertTo_MD_transformation
  for detailed description of this
  * class place in the algorithms hierarchy.
  *
  * @date 11-10-2011
*/

// Class to process event workspace by direct conversion:

class ConvToMDEventsWS : public ConvToMDBase {
public:
  size_t initialize(const MDWSDescription &WSD,
                    boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
                    bool ignoreZeros) override;
  void runConversion(API::Progress *pProgress) override;

protected:
  DataObjects::EventWorkspace_const_sptr m_EventWS;
private:
  // function runs the conversion on
  size_t conversionChunk(size_t workspaceIndex) override;
  // the pointer to the source event workspace as event ws does not work through
  // the public Matrix WS interface
  /**function converts particular type of events into MD space and add these
   * events to the workspace itself    */
  template <class T> size_t convertEventList(size_t workspaceIndex);

  virtual void appendEventsFromInputWS(API::Progress *pProgress, const API::BoxController_sptr &bc);
};



class ConvToMDEventsWSIndexing : public ConvToMDEventsWS {
  enum MD_EVENT_TYPE {
    LEAN,
    REGULAR,
    NONE
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
      throw std::runtime_error("MDD events in md event workspace had an unexpected data type!");
    }
  }

  template<size_t ND, template <size_t> class MDEventType>
  void buildStructureFromSortedEvents(API::Progress *pProgress, const API::BoxController_sptr &bc,
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

      MDEventType<ND> mdEvent = MDEventMaker<ND, MDEventType>::
          makeMDEvent(signal, errorSq, runIndexLoc, detID, &locCoord[0]);


      // Filter events before adding to the ndEvents vector to add in workspace
      // The bounds of the resulting WS have to be already defined
      bool isInOutWSBox = true;
      for(size_t ax = 0; ax < ND; ++ ax) {
        const coord_t& coord{mdEvent.getCenter(ax)};
        if (
         coord < pws->getDimension(ax)->getMinimum() ||
         coord >pws->getDimension(ax)->getMaximum()
        )
          isInOutWSBox = false;
      }

      if(isInOutWSBox)
        mdEventsForSpectrum.emplace_back();
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
void ConvToMDEventsWSIndexing::buildStructureFromSortedEvents(API::Progress *pProgress,
                                                              const API::BoxController_sptr &bc,
                                                              const std::vector<MDEventType<ND>> &mdEvents) {

}



template<typename EventType, size_t ND, template <size_t> class MDEventType>
void ConvToMDEventsWSIndexing::appendEvents(API::Progress *pProgress, const API::BoxController_sptr &bc) {
  std::vector<MDEventType<ND>> mdEvents = convertEvents<EventType, ND, MDEventType>();
  MDSpaceBounds<ND> space;
  const auto& pws{m_OutWSWrapper->pWorkspace()};
  for(size_t ax = 0; ax < ND; ++ ax) {
    space(ax, 0) = pws->getDimension(ax)->getMinimum();
    space(ax, 1) = pws->getDimension(ax)->getMaximum();
  }
#pragma omp parallel for
  for(size_t i = 0; i < mdEvents.size(); ++i)
    mdEvents[i].retrieveIndex(space);
  tbb::parallel_sort(mdEvents.begin(), mdEvents.end(), [] (const MDEventType<ND>& a, const MDEventType<ND>& b) {
    return a.getIndex() < b.getIndex();
  });

  buildStructureFromSortedEvents(pProgress, bc, mdEvents);
}


template <size_t ND>
struct ConvToMDEventsWSIndexing::MDEventMaker<ND, Mantid::DataObjects::MDLeanEvent> {
  static Mantid::DataObjects::MDLeanEvent<ND> makeMDEvent(const double &sig,
                                                         const double &err,
                                                         const uint16_t &run_index,
                                                         const uint32_t &det_id,
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

} // namespace MDAlgorithms
} // namespace Mantid

#endif
