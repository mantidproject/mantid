#include "MantidDataHandling/ParallelEventLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidTypes/Event/TofEvent.h"

namespace Mantid {
namespace DataHandling {

std::vector<int32_t> bankOffsets(const API::ExperimentInfo &ws,
                                 const std::vector<std::string> &bankNames) {
  const auto instrument = ws.getInstrument();
  const auto &compInfo = ws.componentInfo();
  const auto &detInfo = ws.detectorInfo();
  const auto &detIDs = detInfo.detectorIDs();
  const auto monitors = instrument->getMonitors();
  int32_t monitorOffset = static_cast<int32_t>(monitors.size());
  std::vector<int32_t> bankOffsets;
  for (const auto &bankName : bankNames) {
    // Removing "_events" from bankName
    auto bank =
        instrument->getComponentByName(bankName.substr(0, bankName.size() - 7));
    if (bank) {
      const auto &detectors =
          compInfo.detectorsInSubtree(compInfo.indexOf(bank->getComponentID()));
      const size_t detIndex = detectors.front();
      bankOffsets.push_back(detIDs[detIndex] - static_cast<int32_t>(detIndex) +
                            monitorOffset);
      printf("%s %d %d %lu offset %d\n", bankName.c_str(),
             detIDs[detectors.front()], detIDs[detectors.back()],
             detectors.size(), bankOffsets.back());
      if ((detIDs[detectors.back()] - detIDs[detectors.front()]) !=
          static_cast<int32_t>(detectors.size()) - 1)
        throw std::runtime_error("Detector ID range in bank is not contiguous. "
                                 "Cannot use ParallelEventLoader.");
    } else {
      printf("bank %s not found\n", bankName.c_str());
      throw std::runtime_error("");
    }
  }
  return bankOffsets;
}

void ParallelEventLoader::load(DataObjects::EventWorkspace &ws,
                               const std::string &filename,
                               const std::string &groupName,
                               const std::vector<std::string> &bankNames) {
  const size_t size = ws.getNumberHistograms();
  std::vector<std::vector<Types::Event::TofEvent> *> eventLists(size, nullptr);
  for (size_t i = 0; i < size; ++i)
    DataObjects::getEventsFrom(ws.getSpectrum(i), eventLists[i]);

  fprintf(stderr, "loading into %lu event lists\n", size);
  Parallel::IO::EventLoader::load(filename, groupName, bankNames,
                                  bankOffsets(ws, bankNames),
                                  std::move(eventLists));
  //for (size_t i = 0; i < size; ++i)
  //  fprintf(stderr, "total %lu\n", ws.getSpectrum(i).getNumberEvents());
  fprintf(stderr, "loading done\n");
}

/*
for chunk in myfile:
    if currently_processed_buffer_start == buffer_index:
        consumer.wait()
    # ring buffer, loader is taking small steps
    loader.load(chunk, buffer_index)
    buffer_index = (buffer_index + 1)%buffer_count
    if buffer_index * chunk.size <= consumer.optimal_buffer_size:
        # consumer taking large steps (better for MPI), chasing the loader in the ring buffer
        consumer.start_processing(buffer_index_start, buffer_index)
        currently_processed_buffer_start = buffer_index_start
        buffer_index_start = buffer_index
    consumer.finalize()
    */

} // namespace DataHandling
} // namespace Mantid
