// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include <H5Cpp.h>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

namespace NxsFieldNames {
const std::string TIME_OF_FLIGHT("event_time_offset"); // float32 in ORNL nexus files
const std::string DETID("event_id");                   // uint32 in ORNL nexus files
const std::string INDEX_ID("event_index");
} // namespace NxsFieldNames

using PulseROI = std::pair<size_t, size_t>;     // start and stop indices for the pulse ROIs
using EventROI = std::pair<uint64_t, uint64_t>; // start and stop indices for the events ROIs

class NexusLoader {
public:
  NexusLoader(const bool is_time_filtered, const std::vector<PulseROI> &pulse_indices,
              const std::vector<std::pair<int, PulseROI>> &target_to_pulse_indices = {});
  template <typename Type>
  void loadData(H5::DataSet &SDS, std::unique_ptr<std::vector<Type>> &data, const std::vector<size_t> &offsets,
                const std::vector<size_t> &slabsizes);
  std::stack<EventROI> getEventIndexRanges(H5::Group &event_group, const uint64_t number_events);
  std::stack<std::pair<int, EventROI>> getEventIndexSplitRanges(H5::Group &event_group, const uint64_t number_events);
  void loadEventIndex(H5::Group &event_group, std::unique_ptr<std::vector<uint64_t>> &data);

private:
  const bool m_is_time_filtered;
  const std::vector<PulseROI> m_pulse_indices;
  std::vector<std::pair<int, PulseROI>> m_target_to_pulse_indices;
};

} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
