// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Axis.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/EventWorkspaceCollection.h"

class BankPulseTimes;

namespace Mantid {
namespace DataHandling {
class LoadEventNexus;

/** Helper class for LoadEventNexus that is specific to the current default
  loading code for NXevent_data entries in Nexus files, in particular
  LoadBankFromDiskTask and ProcessBankData.
*/
class MANTID_DATAHANDLING_DLL DefaultEventLoader {
public:
  static void
  load(LoadEventNexus *alg, EventWorkspaceCollection &ws, bool haveWeights,
       bool isEventIDSpec, std::vector<std::string> bankNames,
       const std::vector<int> &periodLog, const std::string &classType,
       std::vector<std::size_t> bankNumEvents, const bool oldNeXusFileNames,
       const bool precount, const int chunk, const int totalChunks);

private:
  /// Flag for dealing with a simulated file
  bool m_haveWeights;

  /// True if the event ID is spectrum no not pixel ID
  bool m_isEventIDSpec;

  /// whether or not to launch multiple ProcessBankData jobs per bank
  bool m_splitProcessing;

  /// Do we pre-count the # of events in each pixel ID?
  bool m_precount;

  /// Offset in the pixelID_to_wi_vector to use.
  detid_t m_detIDtoIndexOffset;

  /// Maximum (inclusive) event ID possible for this instrument
  int32_t m_maxEventID{0};

  /// chunk number
  int m_chunkNumber;
  /// number of chunks
  int m_numberOfChunks;
  /// for multiple chunks per bank
  int m_firstChunkForBank;
  /// number of chunks per bank
  size_t m_eventsPerChunk;

  LoadEventNexus *m_loadAlgorithm;
  EventWorkspaceCollection &m_ws;

  /// Vector where index = event_id; value = ptr to std::vector<TofEvent> in the
  /// event list.
  std::vector<std::vector<std::vector<Mantid::Types::Event::TofEvent> *>>
      m_eventVectors;

  /// Vector where index = event_id; value = ptr to std::vector<WeightedEvent>
  /// in the event list.
  std::vector<std::vector<std::vector<Mantid::DataObjects::WeightedEvent> *>>
      m_weightedEventVectors;

  /// Vector where (index = pixel ID+pixelID_to_wi_offset), value = workspace
  /// index)
  std::vector<size_t> m_detIDtoIndexVector;

  /// One entry of pulse times for each preprocessor
  std::vector<boost::shared_ptr<BankPulseTimes>> m_bankPulseTimes;

  DefaultEventLoader(LoadEventNexus *alg, EventWorkspaceCollection &ws,
                     bool haveWeights, bool event_id_is_spec,
                     const size_t numBanks, const bool precount,
                     const int chunk, const int totalChunks);
  std::pair<size_t, size_t>
  setupChunking(std::vector<std::string> &bankNames,
                std::vector<std::size_t> &bankNumEvents);
  /// Map detector IDs to event lists.
  template <class T>
  void makeMapToEventLists(std::vector<std::vector<T>> &eventVectors);
  std::pair<int32_t, int32_t> getMinMaxDetID() const;

  friend class ProcessBankData;
  friend class LoadBankFromDiskTask;
};

/** Generate a look-up table where the index = the pixel ID of an event
 * and the value = a pointer to the EventList in the workspace
 * @param eventVectors :: the array to create the map on
 */
template <class T>
void DefaultEventLoader::makeMapToEventLists(
    std::vector<std::vector<T>> &eventVectors) {
  eventVectors.resize(m_ws.nPeriods());
  if (m_isEventIDSpec) {
    // Find max spectrum no
    auto *ax1 = m_ws.getAxis(1);
    specnum_t maxSpecNo =
        -std::numeric_limits<specnum_t>::max(); // So that any number will be
                                                // greater than this
    for (size_t i = 0; i < ax1->length(); i++) {
      specnum_t spec = ax1->spectraNo(i);
      if (spec > maxSpecNo)
        maxSpecNo = spec;
    }

    // These are used by the bank loader to figure out where to put the events
    // The index of eventVectors is a spectrum number so it is simply resized to
    // the maximum
    // possible spectrum number
    m_maxEventID = maxSpecNo;
    for (size_t i = 0; i < eventVectors.size(); ++i)
      eventVectors[i].resize(maxSpecNo + 1, nullptr);

    for (size_t period = 0; period < m_ws.nPeriods(); ++period) {
      for (size_t i = 0; i < m_ws.getNumberHistograms(); ++i) {
        const auto &spec = m_ws.getSpectrum(i);
        getEventsFrom(m_ws.getSpectrum(i, period),
                      eventVectors[period][spec.getSpectrumNo()]);
      }
    }
  } else {
    auto [minEventID, maxEventID] = getMinMaxDetID();
    m_maxEventID = maxEventID;

    // Make an array where index = pixel ID
    // Set the value to NULL by default
    for (size_t i = 0; i < eventVectors.size(); ++i)
      eventVectors[i].resize(m_maxEventID - minEventID + 1, nullptr);

    for (int32_t j = minEventID; j <= m_maxEventID; ++j) {
      auto index = j + m_detIDtoIndexOffset;
      size_t wi = m_detIDtoIndexVector[index];
      // Save a POINTER to the vector
      if (wi < m_ws.getNumberHistograms()) {
        for (size_t period = 0; period < m_ws.nPeriods(); ++period) {
          getEventsFrom(m_ws.getSpectrum(wi, period),
                        eventVectors[period][index]);
        }
      }
    }
  }
}

} // namespace DataHandling
} // namespace Mantid
