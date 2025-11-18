// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/NexusLoader.h"
#include "MantidNexus/H5Util.h"
#include <ranges>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

NexusLoader::NexusLoader(const bool is_time_filtered, const std::vector<PulseROI> &pulse_indices,
                         const std::vector<std::pair<int, PulseROI>> &target_to_pulse_indices)
    : m_is_time_filtered(is_time_filtered), m_pulse_indices(pulse_indices),
      m_target_to_pulse_indices(target_to_pulse_indices) {}

template <typename Type>
void NexusLoader::loadData(H5::DataSet &SDS, std::unique_ptr<std::vector<Type>> &data,
                           const std::vector<size_t> &offsets, const std::vector<size_t> &slabsizes) {
  // assumes that data is the same type as the dataset
  H5::DataSpace filespace = SDS.getSpace();

  const auto length_actual = static_cast<size_t>(filespace.getSelectNpoints());

  const hsize_t rankedoffset[1] = {static_cast<hsize_t>(offsets[0])};
  const hsize_t rankedextent[1] = {static_cast<hsize_t>(slabsizes[0])};

  size_t total_size = slabsizes[0];

  // only select hyperslab if not loading all the data
  if (rankedextent[0] < length_actual) {
    // set the first hyperslab with H5S_SELECT_SET
    filespace.selectHyperslab(H5S_SELECT_SET, rankedextent, rankedoffset);

    // If more slabs, select them with H5S_SELECT_OR to include in the read.
    // This allows reading non-contiguous data.
    for (size_t i = 1; i < offsets.size(); ++i) {
      const hsize_t offset[1] = {static_cast<hsize_t>(offsets[i])};
      const hsize_t extent[1] = {static_cast<hsize_t>(slabsizes[i])};
      filespace.selectHyperslab(H5S_SELECT_OR, extent, offset);
      total_size += slabsizes[i];
    }
  }

  // create a memory space for the data to read into, total size is all the slabs combined
  const hsize_t total_rankedextent[1] = {static_cast<hsize_t>(total_size)};
  H5::DataSpace memspace(1, total_rankedextent);

  // do the actual read
  const H5::DataType dataType = SDS.getDataType();

  std::size_t dataSize = filespace.getSelectNpoints();
  data->resize(dataSize);
  SDS.read(data->data(), dataType, memspace, filespace);
}

std::stack<EventROI> NexusLoader::getEventIndexRanges(H5::Group &event_group, const uint64_t number_events,
                                                      std::unique_ptr<std::vector<uint64_t>> *event_index_out) {
  // This will return a stack of pairs, where each pair is the start and stop index of the event ranges
  std::stack<EventROI> ranges;
  if (m_is_time_filtered) {
    // TODO this should be made smarter to only read the necessary range
    std::unique_ptr<std::vector<uint64_t>> event_index = std::make_unique<std::vector<uint64_t>>();
    this->loadEventIndex(event_group, event_index);

    // add backwards so that the first range is on top
    for (const auto &pair : m_pulse_indices | std::views::reverse) {
      uint64_t start_event = event_index->at(pair.first);
      uint64_t stop_event =
          (pair.second == std::numeric_limits<size_t>::max()) ? number_events : event_index->at(pair.second);
      if (start_event < stop_event)
        ranges.emplace(start_event, stop_event);
    }

    // If caller wants the event_index data, transfer it
    if (event_index_out) {
      *event_index_out = std::move(event_index);
    }
  } else {
    constexpr uint64_t START_DEFAULT = 0;
    ranges.emplace(START_DEFAULT, number_events);
  }
  return ranges;
}

std::stack<std::pair<int, EventROI>> NexusLoader::getEventIndexSplitRanges(H5::Group &event_group,
                                                                           const uint64_t number_events) {
  // This will return a stack of pairs, where each pair is the start and stop index of the event ranges with a mapping
  // to target, taking intersection with m_pulse_indices
  std::stack<std::pair<int, EventROI>> ranges;
  // TODO this should be made smarter to only read the necessary range
  std::unique_ptr<std::vector<uint64_t>> event_index = std::make_unique<std::vector<uint64_t>>();
  this->loadEventIndex(event_group, event_index);

  // add backwards so that the first range is on top
  for (const auto &pair : m_target_to_pulse_indices | std::views::reverse) {
    uint64_t start_event = event_index->at(pair.second.first);
    uint64_t stop_event = (pair.second.second == std::numeric_limits<size_t>::max())
                              ? number_events
                              : event_index->at(pair.second.second);
    if (start_event < stop_event) {
      ranges.emplace(pair.first, EventROI(start_event, stop_event));
    }
  }

  return ranges;
}

void NexusLoader::loadEventIndex(H5::Group &event_group, std::unique_ptr<std::vector<uint64_t>> &data) {
  auto index_SDS = event_group.openDataSet(NxsFieldNames::INDEX_ID);
  Nexus::H5Util::readArray1DCoerce(index_SDS, *data);
}

// explict instantiation for uint32_t and float
template void NexusLoader::loadData<uint32_t>(H5::DataSet &SDS, std::unique_ptr<std::vector<uint32_t>> &data,
                                              const std::vector<size_t> &offsets, const std::vector<size_t> &slabsizes);
template void NexusLoader::loadData<float>(H5::DataSet &SDS, std::unique_ptr<std::vector<float>> &data,
                                           const std::vector<size_t> &offsets, const std::vector<size_t> &slabsizes);

}; // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
