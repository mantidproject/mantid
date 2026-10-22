// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/LoadEventNexusAsHistogram.h"

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataHandling/LoadEventNexusIndexSetup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidNexus/H5Util.h"
#include "MantidNexus/NexusDescriptor.h"

#include <H5Cpp.h>
#include <tbb/blocked_range.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_pipeline.h>
#include <tbb/task_arena.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Mantid::DataHandling {
namespace {

using API::MatrixWorkspace_sptr;
using DataObjects::Workspace2D;

constexpr size_t DEFAULT_READ_SIZE = 1'048'576;
constexpr size_t PARALLEL_GRAIN_SIZE = 65'536;
constexpr size_t PIPELINE_MEMORY_BUDGET = 256 * 1024 * 1024;

enum class TimeStorage { Int8, UInt8, Int16, UInt16, Int32, UInt32, Int64, UInt64, Float, Double };

using TimeValues = std::variant<std::vector<int8_t>, std::vector<uint8_t>, std::vector<int16_t>, std::vector<uint16_t>,
                                std::vector<int32_t>, std::vector<uint32_t>, std::vector<int64_t>,
                                std::vector<uint64_t>, std::vector<float>, std::vector<double>>;

struct EventChunk {
  explicit EventChunk(const TimeStorage storage) : eventTimes(makeTimeValues(storage)) {}

  static TimeValues makeTimeValues(const TimeStorage storage) {
    switch (storage) {
    case TimeStorage::Int8:
      return std::vector<int8_t>{};
    case TimeStorage::UInt8:
      return std::vector<uint8_t>{};
    case TimeStorage::Int16:
      return std::vector<int16_t>{};
    case TimeStorage::UInt16:
      return std::vector<uint16_t>{};
    case TimeStorage::Int32:
      return std::vector<int32_t>{};
    case TimeStorage::UInt32:
      return std::vector<uint32_t>{};
    case TimeStorage::Int64:
      return std::vector<int64_t>{};
    case TimeStorage::UInt64:
      return std::vector<uint64_t>{};
    case TimeStorage::Float:
      return std::vector<float>{};
    case TimeStorage::Double:
      return std::vector<double>{};
    }
    throw std::logic_error("Unrecognised event-time storage type.");
  }

  std::vector<uint32_t> eventIDs;
  TimeValues eventTimes;
  std::vector<double> pulseHeights;
  size_t length{0};
};

struct NativeBinning {
  uint64_t start;
  uint64_t width;
  uint64_t end;
};

template <typename TimeType>
std::optional<NativeBinning> nativeBinning(const std::vector<double> &params, const double timeScale) {
  if constexpr (!std::is_unsigned_v<TimeType>) {
    return std::nullopt;
  } else {
    const auto convert = [timeScale](const double value) -> std::optional<TimeType> {
      const long double raw = static_cast<long double>(value) / static_cast<long double>(timeScale);
      const long double rounded = std::nearbyint(raw);
      const long double tolerance = 16.0L * std::numeric_limits<double>::epsilon() * std::max(1.0L, std::abs(raw));
      if (!std::isfinite(raw) || std::abs(raw - rounded) > tolerance ||
          rounded < static_cast<long double>(std::numeric_limits<TimeType>::lowest()) ||
          rounded > static_cast<long double>(std::numeric_limits<TimeType>::max()))
        return std::nullopt;
      return static_cast<TimeType>(rounded);
    };

    const auto start = convert(params[0]);
    const auto width = convert(params[1]);
    const auto end = convert(params[2]);
    if (!start || !width || !end || *width <= 0 || *end <= *start)
      return std::nullopt;

    return NativeBinning{static_cast<uint64_t>(*start), static_cast<uint64_t>(*width), static_cast<uint64_t>(*end)};
  }
}

template <typename TimeType> size_t nativeBin(const TimeType time, const NativeBinning &binning) {
  const auto rawTime = static_cast<uint64_t>(time);
  return static_cast<size_t>((rawTime - binning.start) / binning.width);
}

bool linkExists(const H5::Group &group, const std::string &name) {
  return H5Lexists(group.getId(), name.c_str(), H5P_DEFAULT) > 0;
}

std::string withoutLeadingSlash(std::string value) {
  while (!value.empty() && value.front() == '/')
    value.erase(value.begin());
  return value;
}

std::string entryNameFromAddress(const std::string &address) {
  const auto begin = address.find_first_not_of('/');
  if (begin == std::string::npos)
    return {};
  const auto end = address.find('/', begin);
  return address.substr(begin, end - begin);
}

std::string selectEntryName(const Nexus::NexusDescriptor &descriptor, const std::string &requestedEntry,
                            const std::set<std::string> &eventGroups) {
  if (!requestedEntry.empty()) {
    const auto name = withoutLeadingSlash(requestedEntry);
    if (!descriptor.isEntry('/' + name, "NXentry"))
      throw std::runtime_error("The requested NXentry '" + name + "' was not found in the file.");
    return name;
  }

  if (std::any_of(eventGroups.cbegin(), eventGroups.cend(),
                  [](const auto &address) { return entryNameFromAddress(address) == "raw_data_1"; }))
    return "raw_data_1";
  if (!eventGroups.empty())
    return entryNameFromAddress(*eventGroups.begin());
  throw std::runtime_error("No NXevent_data entries were found in the file.");
}

std::string datasetName(const H5::Group &group, const std::string &preferred, const std::string &fallback) {
  if (linkExists(group, preferred))
    return preferred;
  if (!fallback.empty() && linkExists(group, fallback))
    return fallback;
  throw std::runtime_error("NXevent_data group '" + group.getObjName() + "' has no '" + preferred + "' dataset.");
}

size_t datasetSize(const H5::DataSet &dataset) {
  const auto space = dataset.getSpace();
  if (space.getSimpleExtentNdims() != 1)
    throw std::runtime_error("Event datasets must be one-dimensional.");
  return static_cast<size_t>(space.getSimpleExtentNpoints());
}

std::string normalizedUnit(std::string unit) {
  unit.erase(std::remove_if(unit.begin(), unit.end(), [](const unsigned char value) { return std::isspace(value); }),
             unit.end());
  std::transform(unit.begin(), unit.end(), unit.begin(),
                 [](const unsigned char value) { return static_cast<char>(std::tolower(value)); });
  return unit;
}

double microsecondsPerFileUnit(const H5::DataSet &timeDataset) {
  if (!Nexus::H5Util::hasAttribute(timeDataset, "units"))
    return 1.0;

  std::string units;
  Nexus::H5Util::readStringAttribute(timeDataset, "units", units);
  units = normalizedUnit(std::move(units));
  if (units == "ns" || units == "nanosecond" || units == "nanoseconds")
    return 1.e-3;
  if (units == "us" || units == "microsecond" || units == "microseconds" || units == "µs")
    return 1.0;
  if (units == "ms" || units == "millisecond" || units == "milliseconds")
    return 1.e3;
  if (units == "s" || units == "second" || units == "seconds")
    return 1.e6;
  throw std::runtime_error("Unsupported event_time_offset unit '" + units + "'.");
}

TimeStorage timeStorage(const H5::DataSet &timeDataset) {
  const auto dataType = timeDataset.getDataType();
  const auto typeClass = dataType.getClass();
  const auto size = dataType.getSize();
  if (typeClass == H5T_INTEGER) {
    const bool isSigned = H5Tget_sign(dataType.getId()) == H5T_SGN_2;
    if (size == 1)
      return isSigned ? TimeStorage::Int8 : TimeStorage::UInt8;
    if (size == 2)
      return isSigned ? TimeStorage::Int16 : TimeStorage::UInt16;
    if (size == 4)
      return isSigned ? TimeStorage::Int32 : TimeStorage::UInt32;
    if (size == 8)
      return isSigned ? TimeStorage::Int64 : TimeStorage::UInt64;
  } else if (typeClass == H5T_FLOAT) {
    if (size == 4)
      return TimeStorage::Float;
    if (size == 8)
      return TimeStorage::Double;
  }
  throw std::runtime_error("event_time_offset must contain 8-, 16-, 32-, or 64-bit integers, or 32- or "
                           "64-bit floating-point values.");
}

size_t timeStorageSize(const TimeStorage storage) {
  return std::visit([](const auto &values) { return sizeof(typename std::decay_t<decltype(values)>::value_type); },
                    EventChunk::makeTimeValues(storage));
}

void readChunk(EventChunk &chunk, const H5::DataSet &idDataset, const H5::DataSet &timeDataset,
               const H5::DataSet &pulseHeightDataset, const bool hasPulseHeight, const size_t length,
               const size_t offset) {
  Nexus::H5Util::readArray1DCoerce(idDataset, chunk.eventIDs, length, offset);
  std::visit([&](auto &eventTimes) { Nexus::H5Util::readArray1DCoerce(timeDataset, eventTimes, length, offset); },
             chunk.eventTimes);
  if (hasPulseHeight)
    Nexus::H5Util::readArray1DCoerce(pulseHeightDataset, chunk.pulseHeights, length, offset);
  else
    chunk.pulseHeights.clear();
  chunk.length = length;
}

bool canUseNativeBinning(const TimeStorage storage, const std::vector<double> &params, const double timeScale) {
  return std::visit(
      [&](const auto &values) {
        using TimeType = typename std::decay_t<decltype(values)>::value_type;
        return nativeBinning<TimeType>(params, timeScale).has_value();
      },
      EventChunk::makeTimeValues(storage));
}

struct ContiguousEventMapping {
  int offset;
  int64_t operator()(const uint32_t eventID) const { return static_cast<int64_t>(eventID) - offset; }
};

uint32_t mappingEventID(const int64_t id) {
  if (id < 0 || id > std::numeric_limits<uint32_t>::max())
    throw std::runtime_error("Instrument mapping requires non-negative, 32-bit event IDs.");
  return static_cast<uint32_t>(id);
}

class InstrumentEventMapping {
public:
  InstrumentEventMapping(const detid2index_map &workspaceMapping, const std::vector<uint32_t> &monitorEventIDs) {
    std::unordered_map<uint32_t, int64_t> indices;
    for (const auto id : monitorEventIDs)
      indices.emplace(id, -1);
    for (const auto &[eventID, index] : workspaceMapping)
      indices[mappingEventID(eventID)] = static_cast<int64_t>(index);
    if (indices.empty())
      throw std::runtime_error("The instrument mapping contains no event IDs.");
    const auto [min, max] = std::minmax_element(indices.cbegin(), indices.cend(),
                                                [](const auto &a, const auto &b) { return a.first < b.first; });
    m_minID = min->first;
    const uint64_t span = static_cast<uint64_t>(max->first) - m_minID + 1;
    constexpr size_t maxDenseBytes = 64 * 1024 * 1024;
    if (span <= 8 * indices.size() && span <= maxDenseBytes / sizeof(int64_t)) {
      m_dense.assign(static_cast<size_t>(span), UNKNOWN);
      for (const auto &[eventID, index] : indices)
        m_dense[eventID - m_minID] = index;
    } else {
      m_sparse = std::move(indices);
    }
  }

  int64_t operator()(const uint32_t eventID) const {
    int64_t index = UNKNOWN;
    if (!m_dense.empty()) {
      if (eventID >= m_minID && static_cast<uint64_t>(eventID) - m_minID < m_dense.size())
        index = m_dense[eventID - m_minID];
    } else if (const auto found = m_sparse.find(eventID); found != m_sparse.end()) {
      index = found->second;
    }
    if (index == UNKNOWN)
      throw std::runtime_error("Event ID " + std::to_string(eventID) + " is not present in the instrument mapping.");
    return index;
  }

private:
  static constexpr int64_t UNKNOWN = -2;
  uint32_t m_minID{0};
  std::vector<int64_t> m_dense;
  std::unordered_map<uint32_t, int64_t> m_sparse;
};

template <typename CounterType, typename TimeType, typename EventMapping>
void histogramRange(std::vector<CounterType> &counts, const EventChunk &chunk, const std::vector<TimeType> &eventTimes,
                    const size_t begin, const size_t end, const size_t numberOfSpectra, const size_t numberOfBins,
                    const EventMapping &eventMapping, const double minimumPulseHeight, const bool hasPulseHeight,
                    const std::vector<double> &params, const double timeScale, const bool useNativeBinning) {
  if (useNativeBinning) {
    const auto binning = nativeBinning<TimeType>(params, timeScale);
    if (!binning)
      throw std::logic_error("Native time binning was selected for an incompatible event-time dataset.");
    for (size_t eventIndex = begin; eventIndex != end; ++eventIndex) {
      if (hasPulseHeight && !(chunk.pulseHeights[eventIndex] > minimumPulseHeight))
        continue;
      const int64_t workspaceIndex = eventMapping(chunk.eventIDs[eventIndex]);
      if (workspaceIndex < 0 || workspaceIndex >= static_cast<int64_t>(numberOfSpectra))
        continue;

      const auto rawTime = static_cast<uint64_t>(eventTimes[eventIndex]);
      if (rawTime < binning->start || rawTime >= binning->end)
        continue;
      const auto bin = nativeBin(eventTimes[eventIndex], *binning);
      if (bin < numberOfBins)
        ++counts[static_cast<size_t>(workspaceIndex) * numberOfBins + bin];
    }
  } else {
    for (size_t eventIndex = begin; eventIndex != end; ++eventIndex) {
      if (hasPulseHeight && !(chunk.pulseHeights[eventIndex] > minimumPulseHeight))
        continue;
      const int64_t workspaceIndex = eventMapping(chunk.eventIDs[eventIndex]);
      if (workspaceIndex < 0 || workspaceIndex >= static_cast<int64_t>(numberOfSpectra))
        continue;

      const double time = static_cast<double>(eventTimes[eventIndex]) * timeScale;
      if (time < params[0] || time >= params[2])
        continue;
      const auto bin = static_cast<size_t>((time - params[0]) / params[1]);
      if (bin < numberOfBins)
        ++counts[static_cast<size_t>(workspaceIndex) * numberOfBins + bin];
    }
  }
}

void validateSinglePeriod(const H5::Group &eventGroup) {
  if (!linkExists(eventGroup, "period_number"))
    return;

  const auto periods = Nexus::H5Util::readArray1DCoerce<uint32_t>(eventGroup, "period_number");
  if (!periods.empty() && std::any_of(periods.cbegin() + 1, periods.cend(),
                                      [first = periods.front()](const auto value) { return value != first; }))
    throw std::runtime_error("LoadEventNexusAsHistogram does not support multiple periods.");
}

std::vector<std::string> eventGroupsInEntry(const std::set<std::string> &allEventGroups, const std::string &entryName) {
  const std::string prefix = '/' + entryName + '/';
  std::vector<std::string> result;
  std::copy_if(allEventGroups.cbegin(), allEventGroups.cend(), std::back_inserter(result),
               [&prefix](const auto &address) {
                 return address.starts_with(prefix) && !address.ends_with("bank_error_events") &&
                        !address.ends_with("bank_unmapped_events");
               });
  if (result.empty())
    throw std::runtime_error("No NXevent_data entries were found below NXentry '" + entryName + "'.");
  return result;
}

struct HistogramOptions {
  size_t numberOfSpectra;
  std::vector<double> params;
  int eventIDOffset;
  double minimumPulseHeight;
  size_t readSize;
  std::string processingStrategy;
  size_t maxInFlightChunks;
  size_t numberOfBins{0};
  bool filterByPulseHeight{true};
  std::optional<InstrumentEventMapping> instrumentMapping;
};

HistogramOptions readHistogramOptions(const API::Algorithm &algorithm) {
  return {static_cast<size_t>(static_cast<int>(algorithm.getProperty("NumberOfSpectra"))),
          algorithm.getProperty("Params"),
          algorithm.getProperty("EventIDOffset"),
          algorithm.getProperty("MinimumPulseHeight"),
          static_cast<size_t>(static_cast<int>(algorithm.getProperty("ReadSizeFromDisk"))),
          algorithm.getPropertyValue("ProcessingStrategy"),
          static_cast<size_t>(static_cast<int>(algorithm.getProperty("MaxInFlightChunks"))),
          0,
          algorithm.getProperty("FilterByPulseHeight"),
          std::nullopt};
}

struct EventGroup {
  H5::DataSet idDataset;
  H5::DataSet timeDataset;
  H5::DataSet pulseHeightDataset;
  size_t numberOfEvents;
  bool hasPulseHeight;
  TimeStorage storage;
  double timeScale;
  bool useNativeBinning;
};

EventGroup openEventGroup(const H5::H5File &file, const std::string &groupAddress, const HistogramOptions &options) {
  const auto eventGroup = file.openGroup(groupAddress);
  validateSinglePeriod(eventGroup);

  const auto idName = datasetName(eventGroup, "event_id", "event_pixel_id");
  const auto timeName = datasetName(eventGroup, "event_time_offset", "event_time_of_flight");
  auto idDataset = eventGroup.openDataSet(idName);
  auto timeDataset = eventGroup.openDataSet(timeName);
  const size_t numberOfEvents = datasetSize(idDataset);
  if (datasetSize(timeDataset) != numberOfEvents)
    throw std::runtime_error("event_id and event_time_offset have different lengths in '" + groupAddress + "'.");

  const bool hasPulseHeight = options.filterByPulseHeight && linkExists(eventGroup, "pulse_height");
  H5::DataSet pulseHeightDataset;
  if (hasPulseHeight) {
    pulseHeightDataset = eventGroup.openDataSet("pulse_height");
    if (datasetSize(pulseHeightDataset) != numberOfEvents)
      throw std::runtime_error("pulse_height has a different length from event_id in '" + groupAddress + "'.");
  }

  const auto storage = timeStorage(timeDataset);
  const double timeScale = microsecondsPerFileUnit(timeDataset);

  return {std::move(idDataset),
          std::move(timeDataset),
          std::move(pulseHeightDataset),
          numberOfEvents,
          hasPulseHeight,
          storage,
          timeScale,
          canUseNativeBinning(storage, options.params, timeScale)};
}

struct ProcessingPlan {
  bool usePipeline;
  size_t maxInFlightChunks;
};

ProcessingPlan chooseProcessingPlan(const EventGroup &group, const HistogramOptions &options) {
  const size_t numberOfChunks = (group.numberOfEvents + options.readSize - 1) / options.readSize;
  const size_t bytesPerChunk = options.readSize * (sizeof(uint32_t) + timeStorageSize(group.storage) +
                                                   (group.hasPulseHeight ? sizeof(double) : 0));
  const size_t availableConcurrency = static_cast<size_t>(std::max(1, tbb::this_task_arena::max_concurrency()));
  const size_t chunksWithinBudget = std::max<size_t>(1, PIPELINE_MEMORY_BUDGET / std::max<size_t>(1, bytesPerChunk));
  const size_t automaticTokens = std::min(availableConcurrency, chunksWithinBudget);
  const size_t requestedTokens = options.maxInFlightChunks == 0 ? automaticTokens : options.maxInFlightChunks;
  const size_t maxTokens = std::max<size_t>(1, std::min(requestedTokens, std::max<size_t>(1, numberOfChunks)));
  const bool usePipeline = options.processingStrategy == "Pipeline" ||
                           (options.processingStrategy == "Auto" && numberOfChunks > 1 && maxTokens > 1);
  return {usePipeline, maxTokens};
}

uint64_t countInputEvents(const H5::H5File &file, const std::vector<std::string> &groupAddresses) {
  uint64_t totalEvents = 0;
  for (const auto &groupAddress : groupAddresses) {
    const auto eventGroup = file.openGroup(groupAddress);
    const auto idName = datasetName(eventGroup, "event_id", "event_pixel_id");
    const auto numberOfEvents = datasetSize(eventGroup.openDataSet(idName));
    if (numberOfEvents > std::numeric_limits<uint64_t>::max() - totalEvents)
      throw std::runtime_error("The total number of events cannot be represented by a 64-bit counter.");
    totalEvents += static_cast<uint64_t>(numberOfEvents);
  }
  return totalEvents;
}

template <typename CounterType> using ThreadLocalCounts = tbb::enumerable_thread_specific<std::vector<CounterType>>;

template <typename CounterType, typename TimeType>
void histogramEventRange(std::vector<CounterType> &counts, const EventChunk &chunk,
                         const std::vector<TimeType> &eventTimes, const size_t begin, const size_t end,
                         const EventGroup &group, const HistogramOptions &options) {
  const auto histogram = [&](const auto &mapping) {
    histogramRange(counts, chunk, eventTimes, begin, end, options.numberOfSpectra, options.numberOfBins, mapping,
                   options.minimumPulseHeight, group.hasPulseHeight, options.params, group.timeScale,
                   group.useNativeBinning);
  };
  // Dispatch once per range so the default event loop retains arithmetic-only mapping.
  if (options.instrumentMapping)
    histogram(*options.instrumentMapping);
  else
    histogram(ContiguousEventMapping{options.eventIDOffset});
}

template <typename CounterType>
void histogramChunk(std::vector<CounterType> &counts, const EventChunk &chunk, const EventGroup &group,
                    const HistogramOptions &options) {
  std::visit(
      [&](const auto &eventTimes) { histogramEventRange(counts, chunk, eventTimes, 0, chunk.length, group, options); },
      chunk.eventTimes);
}

// HDF5 reads are serial; only the histogramming stage runs concurrently.
template <typename CounterType>
uint64_t processPipeline(const EventGroup &group, const HistogramOptions &options, const size_t maxTokens,
                         ThreadLocalCounts<CounterType> &threadCounts) {
  size_t nextOffset = 0;
  uint64_t eventsRead = 0;
  std::vector<std::unique_ptr<EventChunk>> availableChunks;
  std::mutex availableChunksMutex;

  const auto readNextChunk = [&](tbb::flow_control &flowControl) {
    if (nextOffset >= group.numberOfEvents) {
      flowControl.stop();
      return std::unique_ptr<EventChunk>{};
    }

    std::unique_ptr<EventChunk> chunk;
    {
      const std::scoped_lock lock(availableChunksMutex);
      if (!availableChunks.empty()) {
        chunk = std::move(availableChunks.back());
        availableChunks.pop_back();
      }
    }
    if (!chunk)
      chunk = std::make_unique<EventChunk>(group.storage);

    const size_t length = std::min(options.readSize, group.numberOfEvents - nextOffset);
    readChunk(*chunk, group.idDataset, group.timeDataset, group.pulseHeightDataset, group.hasPulseHeight, length,
              nextOffset);
    nextOffset += length;
    eventsRead += length;
    return chunk;
  };

  const auto histogramNextChunk = [&](std::unique_ptr<EventChunk> chunk) {
    histogramChunk(threadCounts.local(), *chunk, group, options);
    const std::scoped_lock lock(availableChunksMutex);
    availableChunks.emplace_back(std::move(chunk));
  };

  tbb::parallel_pipeline(
      maxTokens,
      tbb::make_filter<void, std::unique_ptr<EventChunk>>(tbb::filter_mode::serial_in_order, readNextChunk) &
          tbb::make_filter<std::unique_ptr<EventChunk>, void>(tbb::filter_mode::parallel, histogramNextChunk));
  return eventsRead;
}

template <typename CounterType>
uint64_t processParallelEvents(const EventGroup &group, const HistogramOptions &options,
                               ThreadLocalCounts<CounterType> &threadCounts) {
  uint64_t eventsRead = 0;
  EventChunk chunk(group.storage);
  for (size_t offset = 0; offset < group.numberOfEvents; offset += options.readSize) {
    const size_t length = std::min(options.readSize, group.numberOfEvents - offset);
    readChunk(chunk, group.idDataset, group.timeDataset, group.pulseHeightDataset, group.hasPulseHeight, length,
              offset);
    std::visit(
        [&](const auto &eventTimes) {
          tbb::parallel_for(tbb::blocked_range<size_t>(0, length, PARALLEL_GRAIN_SIZE),
                            [&](const tbb::blocked_range<size_t> &range) {
                              histogramEventRange(threadCounts.local(), chunk, eventTimes, range.begin(), range.end(),
                                                  group, options);
                            });
        },
        chunk.eventTimes);
    eventsRead += length;
  }
  return eventsRead;
}

struct HistogramResult {
  std::vector<uint64_t> counts;
  uint64_t eventsRead;
};

template <typename CounterType>
HistogramResult histogramGroups(const H5::H5File &file, const std::vector<std::string> &groupAddresses,
                                const HistogramOptions &options, API::Progress &progress) {
  const size_t histogramSize = options.numberOfSpectra * options.numberOfBins;
  ThreadLocalCounts<CounterType> threadCounts([histogramSize]() { return std::vector<CounterType>(histogramSize, 0); });
  uint64_t eventsRead = 0;
  for (const auto &groupAddress : groupAddresses) {
    const auto group = openEventGroup(file, groupAddress, options);
    const auto plan = chooseProcessingPlan(group, options);
    if (plan.usePipeline)
      eventsRead += processPipeline(group, options, plan.maxInFlightChunks, threadCounts);
    else
      eventsRead += processParallelEvents(group, options, threadCounts);
    progress.report("Histogramming events");
  }

  std::vector<uint64_t> counts(histogramSize, 0);
  for (const auto &localCounts : threadCounts)
    std::transform(counts.cbegin(), counts.cend(), localCounts.cbegin(), counts.begin(), std::plus<>());
  return {std::move(counts), eventsRead};
}

std::shared_ptr<Workspace2D> createOutputWorkspace(const MatrixWorkspace_sptr &metadataWorkspace,
                                                   const size_t numberOfSpectra, const std::vector<double> &binEdges,
                                                   const std::string &filename) {
  const size_t numberOfBins = binEdges.size() - 1;
  auto workspace = std::dynamic_pointer_cast<Workspace2D>(
      API::WorkspaceFactory::Instance().create(metadataWorkspace, numberOfSpectra, binEdges.size(), numberOfBins));
  const auto sharedX = Kernel::make_cow<HistogramData::HistogramX>(binEdges);
  for (size_t workspaceIndex = 0; workspaceIndex < numberOfSpectra; ++workspaceIndex) {
    workspace->setSharedX(workspaceIndex, sharedX);
    auto &spectrum = workspace->getSpectrum(workspaceIndex);
    spectrum.setSpectrumNo(static_cast<specnum_t>(workspaceIndex + 1));
    spectrum.clearDetectorIDs();
    spectrum.setDetectorID(static_cast<detid_t>(workspaceIndex + 1));
  }
  workspace->getAxis(0)->setUnit("TOF");
  workspace->setYUnit("Counts");
  workspace->mutableRun().addProperty("Filename", filename, true);
  return workspace;
}

uint64_t storeHistogram(const HistogramResult &histogram, Workspace2D &workspace) {
  const size_t numberOfBins = workspace.blocksize();
  uint64_t acceptedEvents = 0;
  for (size_t workspaceIndex = 0; workspaceIndex < workspace.getNumberHistograms(); ++workspaceIndex) {
    auto &y = workspace.mutableY(workspaceIndex);
    auto &e = workspace.mutableE(workspaceIndex);
    for (size_t bin = 0; bin < numberOfBins; ++bin) {
      const auto count = histogram.counts[workspaceIndex * numberOfBins + bin];
      y[bin] = static_cast<double>(count);
      e[bin] = std::sqrt(y[bin]);
      acceptedEvents += count;
    }
  }
  return acceptedEvents;
}

struct InstrumentSetup {
  Indexing::IndexInfo indexInfo;
  std::vector<uint32_t> monitorEventIDs;
};

InstrumentSetup createInstrumentSetup(const MatrixWorkspace_sptr &metadataWorkspace) {
  const auto instrument = metadataWorkspace->getInstrument();
  if (instrument->getDetectorIDs(true).empty())
    throw std::runtime_error("LoadInstrument=True requires a successfully loaded instrument with detectors.");

  LoadEventNexusIndexSetup indexSetup(metadataWorkspace, EMPTY_INT(), EMPTY_INT(), {});
  std::vector<uint32_t> monitorEventIDs;
  for (const auto id : instrument->getMonitorIDs()) {
    if (id >= 0)
      monitorEventIDs.emplace_back(mappingEventID(id));
  }
  return {indexSetup.makeIndexInfo(), std::move(monitorEventIDs)};
}

} // namespace

DECLARE_ALGORITHM(LoadEventNexusAsHistogram)

const std::string LoadEventNexusAsHistogram::name() const { return "LoadEventNexusAsHistogram"; }

int LoadEventNexusAsHistogram::version() const { return 1; }

const std::string LoadEventNexusAsHistogram::category() const { return "DataHandling\\Nexus"; }

const std::string LoadEventNexusAsHistogram::summary() const {
  return "Loads event NeXus data directly into a histogram workspace without constructing an EventWorkspace.";
}

const std::vector<std::string> LoadEventNexusAsHistogram::seeAlso() const {
  return {"LoadEventNexus", "LoadEventAsWorkspace2D", "AlignAndFocusPowderSlim"};
}

void LoadEventNexusAsHistogram::init() {
  const std::vector<std::string> extensions{".nxs.h5", ".nxs", "_event.nxs"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, extensions),
                  "The event NeXus file to load.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<Workspace2D>>("OutputWorkspace", "", Kernel::Direction::Output),
      "The output histogram workspace.");
  declareProperty("NXentryName", "", "Optional name of the NXentry to load.");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(1);
  declareProperty(
      "NumberOfSpectra", 64, positiveInt,
      "Number of output spectra when LoadInstrument=False. When LoadInstrument=True the default is ignored; "
      "a non-default value must match the derived spectrum count.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("Params", std::vector<double>{0.0, 0.016, 32.768},
                                                                  std::make_shared<Kernel::RebinParamsValidator>()),
                  "Linear histogram binning in microseconds: start, width, end.");
  declareProperty("EventIDOffset", 0,
                  "Value subtracted from each event ID to obtain its zero-based output workspace index when "
                  "LoadInstrument=False. Must be zero when LoadInstrument=True.");
  declareProperty("FilterByPulseHeight", true,
                  "Apply MinimumPulseHeight when pulse_height is present. If false, do not read or filter "
                  "pulse_height; detector and time-range selection still apply.");
  declareProperty("MinimumPulseHeight", 0.0,
                  "Events are retained only when pulse_height is strictly greater than this value. The filter is "
                  "not applied when pulse_height is absent.");
  declareProperty("ReadSizeFromDisk", static_cast<int>(DEFAULT_READ_SIZE), positiveInt,
                  "Maximum number of events read from each dataset at a time.");
  declareProperty(
      "ProcessingStrategy", "Auto",
      std::make_shared<Kernel::StringListValidator>(std::vector<std::string>{"Auto", "Pipeline", "ParallelEvents"}),
      "Choose whether to overlap serial NeXus reads with whole-chunk histogramming, divide each chunk "
      "between threads, or select automatically.");
  auto nonNegativeInt = std::make_shared<Kernel::BoundedValidator<int>>();
  nonNegativeInt->setLower(0);
  declareProperty("MaxInFlightChunks", 0, nonNegativeInt,
                  "Maximum number of chunks held by the pipeline. Zero selects a limit from available concurrency "
                  "and a bounded memory budget.");
  declareProperty("LoadInstrument", true,
                  "Load the instrument definition and map physical detector IDs to output spectra. If false, "
                  "use EventIDOffset and NumberOfSpectra to map event IDs arithmetically without an instrument.");
  declareProperty("LoadLogs", true, "Load sample logs into the output workspace.");
  declareProperty("LoadNexusInstrumentXML", true,
                  "When loading the instrument, prefer an instrument definition embedded in the NeXus file.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("LogAllowList"),
                  "If specified, load only these sample logs.");
  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("LogBlockList"),
                  "If specified, do not load these sample logs.");
}

std::map<std::string, std::string> LoadEventNexusAsHistogram::validateInputs() {
  std::map<std::string, std::string> errors;
  if (getProperty("LoadInstrument")) {
    const int offset = getProperty("EventIDOffset");
    if (offset != 0)
      errors["EventIDOffset"] = "EventIDOffset applies only when LoadInstrument=False.";
  }
  const std::vector<double> params = getProperty("Params");
  if (params.size() != 3) {
    errors["Params"] = "Exactly three values are required: start, width, end.";
  } else if (params[1] <= 0.0) {
    errors["Params"] = "Only positive-width linear binning is supported.";
  }

  const std::vector<std::string> allowList = getProperty("LogAllowList");
  const std::vector<std::string> blockList = getProperty("LogBlockList");
  if (!allowList.empty() && !blockList.empty()) {
    errors["LogAllowList"] = "LogAllowList and LogBlockList are mutually exclusive.";
    errors["LogBlockList"] = "LogAllowList and LogBlockList are mutually exclusive.";
  }
  return errors;
}

MatrixWorkspace_sptr LoadEventNexusAsHistogram::loadMetadata(const std::string &filename, const std::string &entryName,
                                                             const Nexus::NexusDescriptor &descriptor) {
  MatrixWorkspace_sptr metadataWorkspace = API::WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  try {
    LoadEventNexus::loadEntryMetadata(filename, metadataWorkspace, entryName);
  } catch (const std::exception &error) {
    g_log.warning() << "Unable to load entry metadata: " << error.what() << '\n';
  }

  if (getProperty("LoadLogs")) {
    int numberOfPeriods = 1;
    auto periodLog = std::make_unique<const Kernel::TimeSeriesProperty<int>>("period_log");
    const std::vector<std::string> allowList = getProperty("LogAllowList");
    const std::vector<std::string> blockList = getProperty("LogBlockList");
    LoadEventNexus::runLoadNexusLogs<MatrixWorkspace_sptr>(filename, metadataWorkspace, *this, false, numberOfPeriods,
                                                           periodLog, allowList, blockList);
    if (numberOfPeriods != 1)
      throw std::runtime_error("LoadEventNexusAsHistogram does not support multiple periods.");
  }

  if (getProperty("LoadInstrument")) {
    if (!LoadEventNexus::loadInstrument<MatrixWorkspace_sptr>(filename, metadataWorkspace, entryName, this,
                                                              &descriptor))
      g_log.warning("The instrument definition could not be loaded.");
  }

  return metadataWorkspace;
}

void LoadEventNexusAsHistogram::exec() {
  const std::string filename = getPropertyValue("Filename");
  const Nexus::NexusDescriptor descriptor(filename);
  const auto allEventGroups = descriptor.allAddressesOfType("NXevent_data");
  const auto entryName = selectEntryName(descriptor, getPropertyValue("NXentryName"), allEventGroups);
  H5::H5File file(filename, H5F_ACC_RDONLY, Nexus::H5Util::defaultFileAcc());
  if (getProperty("LoadInstrument") && linkExists(file.openGroup(entryName), "isis_vms_compat"))
    throw std::runtime_error("LoadInstrument=True does not support legacy isis_vms_compat mapping.");
  const auto groupAddresses = eventGroupsInEntry(allEventGroups, entryName);

  auto options = readHistogramOptions(*this);
  std::vector<double> binEdges;
  Kernel::VectorHelper::createAxisFromRebinParams(options.params, binEdges, true, false);
  options.numberOfBins = binEdges.size() - 1;

  const auto metadataWorkspace = loadMetadata(filename, entryName, descriptor);
  std::optional<InstrumentSetup> instrumentSetup;
  if (getProperty("LoadInstrument")) {
    instrumentSetup.emplace(createInstrumentSetup(metadataWorkspace));
    if (!isDefault("NumberOfSpectra") && options.numberOfSpectra != instrumentSetup->indexInfo.size())
      throw std::runtime_error("NumberOfSpectra does not match the instrument mapping.");
    options.numberOfSpectra = instrumentSetup->indexInfo.size();
  }
  auto outputWorkspace = createOutputWorkspace(metadataWorkspace, options.numberOfSpectra, binEdges, filename);
  if (instrumentSetup) {
    outputWorkspace->setIndexInfo(instrumentSetup->indexInfo);
    options.instrumentMapping.emplace(outputWorkspace->getDetectorIDToWorkspaceIndexMap(),
                                      instrumentSetup->monitorEventIDs);
  }

  API::Progress progress(this, 0.2, 0.95, groupAddresses.size());
  const bool useUInt32 = countInputEvents(file, groupAddresses) <= std::numeric_limits<uint32_t>::max();
  const auto histogram = [&] {
    if (useUInt32)
      return histogramGroups<uint32_t>(file, groupAddresses, options, progress);
    return histogramGroups<uint64_t>(file, groupAddresses, options, progress);
  }();

  const auto acceptedEvents = storeHistogram(histogram, *outputWorkspace);
  g_log.information() << "Histogrammed " << acceptedEvents << " of " << histogram.eventsRead << " events.\n";
  setProperty("OutputWorkspace", outputWorkspace);
}

} // namespace Mantid::DataHandling
