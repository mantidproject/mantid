#include "MantidDataHandling/ParallelEventLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidTypes/SpectrumDefinition.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace DataHandling {

/// Return offset between global spectrum index and detector ID for given banks.
std::vector<int32_t> bankOffsets(const API::ExperimentInfo &ws,
                                 const std::string &filename,
                                 const std::string &groupName,
                                 const std::vector<std::string> &bankNames) {
  // Read an event ID for each bank. This is always a detector ID since the
  // parallel loader is disabled otherwise. It is assumed that detector IDs
  // within a bank are contiguous.
  const auto &idToBank = Parallel::IO::EventLoader::makeAnyEventIdToBankMap(
      filename, groupName, bankNames);

  const auto &detInfo = ws.detectorInfo();
  const auto &detIds = detInfo.detectorIDs();
  int32_t spectrumIndex{0}; // *global* index
  std::vector<int32_t> bankOffsets(bankNames.size(), 0);
  for (size_t i = 0; i < detInfo.size(); ++i) {
    // Used only in LoadEventNexus so we know there is a 1:1 mapping, omitting
    // monitors.
    if (!detInfo.isMonitor(i)) {
      detid_t detId = detIds[i];
      // The offset is the difference between the event ID and the spectrum
      // index and can then be used to translate from the former to the latter
      // by simple subtraction. If no eventId could be read for a bank it
      // implies that there are no events, so any offset will do since it is
      // unused, keeping as initialized to 0 above.
      if (idToBank.count(detId) == 1) {
        size_t bank = idToBank.at(detId);
        bankOffsets[bank] = detId - spectrumIndex;
      }
      spectrumIndex++;
    }
  }
  return bankOffsets;
}

/// Load events from given banks into given EventWorkspace.
void ParallelEventLoader::load(DataObjects::EventWorkspace &ws,
                               const std::string &filename,
                               const std::string &groupName,
                               const std::vector<std::string> &bankNames) {
  const size_t size = ws.getNumberHistograms();
  std::vector<std::vector<Types::Event::TofEvent> *> eventLists(size, nullptr);
  for (size_t i = 0; i < size; ++i)
    DataObjects::getEventsFrom(ws.getSpectrum(i), eventLists[i]);

  Parallel::IO::EventLoader::load(
      ws.indexInfo().communicator(), filename, groupName, bankNames,
      bankOffsets(ws, filename, groupName, bankNames), std::move(eventLists));
}

} // namespace DataHandling
} // namespace Mantid
