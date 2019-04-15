// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/ParallelEventLoader.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidParallel/IO/EventLoader.h"
#include "MantidTypes/Event/TofEvent.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace DataHandling {

/// Return offset between global spectrum index and detector ID for given banks.
std::vector<int32_t> bankOffsets(const API::ExperimentInfo &ws,
                                 const std::string &filename,
                                 const std::string &groupName,
                                 const std::vector<std::string> &bankNames) {
  // Read an event ID for each bank. This is always a detector ID since
  // bankOffsetsSpectrumNumbers is used otherwise. It is assumed that detector
  // IDs within a bank are contiguous.
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

/// Return offset between global spectrum index and spectrum number for given
/// banks.
std::vector<int32_t> bankOffsetsSpectrumNumbers(
    const API::MatrixWorkspace &ws, const std::string &filename,
    const std::string &groupName, const std::vector<std::string> &bankNames) {
  // Read an event ID for each bank. This is always a spectrum number since
  // bankOffsets is used otherwise. It is assumed that spectrum numbers within a
  // bank are contiguous.
  const auto &idToBank = Parallel::IO::EventLoader::makeAnyEventIdToBankMap(
      filename, groupName, bankNames);

  // *Global* vector of spectrum numbers.
  const auto &specNums = ws.indexInfo().spectrumNumbers();
  int32_t spectrumIndex{0}; // *global* index
  std::vector<int32_t> offsets(bankNames.size(), 0);
  for (auto i : specNums) {
    // In contrast to the case of event ID = detector ID we know that any
    // spectrum number has a corresponding event ID, i.e., we do not need
    // special handling for monitors.
    specnum_t specNum = static_cast<specnum_t>(i);
    // See comment in bankOffsets regarding this offset computation.
    if (idToBank.count(specNum) == 1) {
      size_t bank = idToBank.at(specNum);
      offsets[bank] = specNum - spectrumIndex;
    }
    spectrumIndex++;
  }
  return offsets;
}

std::vector<std::vector<Types::Event::TofEvent> *>
getResultVector(DataObjects::EventWorkspace &ws) {
  const size_t size = ws.getNumberHistograms();

  std::vector<std::vector<Types::Event::TofEvent> *> eventLists(size, nullptr);
  for (size_t i = 0; i < size; ++i)
    getEventsFrom(ws.getSpectrum(i), eventLists[i]);
  return eventLists;
}

std::vector<int32_t> getOffsets(const DataObjects::EventWorkspace &ws,
                                const std::string &filename,
                                const std::string &groupName,
                                const std::vector<std::string> &bankNames,
                                const bool eventIDIsSpectrumNumber) {
  const auto offsets =
      eventIDIsSpectrumNumber
          ? bankOffsetsSpectrumNumbers(ws, filename, groupName, bankNames)
          : bankOffsets(ws, filename, groupName, bankNames);
  return offsets;
}

/// Load events from given banks into given EventWorkspace using MPI.
void ParallelEventLoader::loadMPI(DataObjects::EventWorkspace &ws,
                                  const std::string &filename,
                                  const std::string &groupName,
                                  const std::vector<std::string> &bankNames,
                                  const bool eventIDIsSpectrumNumber) {
  std::vector<std::vector<Types::Event::TofEvent> *> eventLists =
      getResultVector(ws);
  std::vector<int32_t> offsets =
      getOffsets(ws, filename, groupName, bankNames, eventIDIsSpectrumNumber);
  Parallel::IO::EventLoader::load(ws.indexInfo().communicator(), filename,
                                  groupName, bankNames, offsets,
                                  std::move(eventLists));
}

/// Load events from given banks into given EventWorkspace using
/// boost::interprocess.
void ParallelEventLoader::loadMultiProcess(
    DataObjects::EventWorkspace &ws, const std::string &filename,
    const std::string &groupName, const std::vector<std::string> &bankNames,
    const bool eventIDIsSpectrumNumber, const bool precalcEvents) {
  auto eventLists = getResultVector(ws);
  std::vector<int32_t> offsets =
      getOffsets(ws, filename, groupName, bankNames, eventIDIsSpectrumNumber);
  Parallel::IO::EventLoader::load(filename, groupName, bankNames, offsets,
                                  std::move(eventLists), precalcEvents);
}

} // namespace DataHandling
} // namespace Mantid
