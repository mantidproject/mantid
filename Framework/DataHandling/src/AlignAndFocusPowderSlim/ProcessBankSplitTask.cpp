// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankSplitTask.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessEventsTask.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ParallelMinMax.h"
#include "MantidKernel/Timer.h"
#include "MantidNexus/H5Util.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

#include <ranges>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

namespace {

// Logger for this class
auto g_log = Kernel::Logger("ProcessBankSplitTask");

} // namespace
ProcessBankSplitTask::ProcessBankSplitTask(std::vector<std::string> &bankEntryNames, H5::H5File &h5file,
                                           std::shared_ptr<NexusLoader> loader, std::vector<int> &workspaceIndices,
                                           std::vector<SpectraProcessingData> &processingDatas,
                                           const BankCalibrationFactory &calibFactory, const size_t events_per_chunk,
                                           const size_t grainsize_event, std::shared_ptr<API::Progress> &progress)
    : ProcessBankTaskBase(bankEntryNames, loader, calibFactory), m_h5file(h5file), m_workspaceIndices(workspaceIndices),
      m_processingDatas(processingDatas), m_events_per_chunk(events_per_chunk), m_grainsize_event(grainsize_event),
      m_progress(progress) {}
void ProcessBankSplitTask::operator()(const tbb::blocked_range<size_t> &range) const {
  auto entry = m_h5file.openGroup("entry"); // type=NXentry
  for (size_t bank_index = range.begin(); bank_index < range.end(); ++bank_index) {
    const auto &bankName = this->bankName(bank_index);
    // empty bank names indicate spectra to skip; control should never get here, but just in case
    if (bankName.empty()) {
      continue;
    }
    Kernel::Timer timer;
    g_log.debug() << bankName << " start" << std::endl;

    // open the bank
    auto event_group = entry.openGroup(bankName); // type=NXevent_data

    // skip empty dataset
    auto tof_SDS = event_group.openDataSet(NxsFieldNames::TIME_OF_FLIGHT);
    const int64_t total_events = static_cast<size_t>(tof_SDS.getSpace().getSelectNpoints());
    if (total_events == 0) {
      m_progress->report();
      continue;
    }

    auto eventSplitRanges = this->getEventIndexSplitRanges(event_group, total_events);

    // get handle to the data
    auto detID_SDS = event_group.openDataSet(NxsFieldNames::DETID);
    // auto tof_SDS = event_group.openDataSet(NxsFieldNames::TIME_OF_FLIGHT);
    // and the units
    std::string tof_unit;
    Nexus::H5Util::readStringAttribute(tof_SDS, "units", tof_unit);
    // now the calibration for the output group can be created
    // which detectors go into the current group - assumes ouput spectrum number is one more than workspace index
    const auto calibrations = this->getCalibrations(tof_unit, bank_index);

    // declare arrays once so memory can be reused
    auto event_detid = std::make_unique<std::vector<uint32_t>>();       // uint32 for ORNL nexus file
    auto event_time_of_flight = std::make_unique<std::vector<float>>(); // float for ORNL nexus files

    // read parts of the bank at a time until all events are processed
    while (!eventSplitRanges.empty()) {
      // Create offsets and slab sizes for the next chunk of events.
      // This will read at most m_events_per_chunk events from the file
      // and will split the ranges if necessary for the next iteration.
      std::vector<size_t> offsets;
      std::vector<size_t> slabsizes;
      std::vector<std::pair<int, EventROI>> relative_target_ranges;

      size_t total_events_to_read = 0;
      // Process the event ranges until we reach the desired number of events to read or run out of ranges
      while (!eventSplitRanges.empty() && total_events_to_read < m_events_per_chunk) {
        // Get the next event range from the stack
        auto [target, eventRange] = eventSplitRanges.top();
        eventSplitRanges.pop();

        size_t range_size = eventRange.second - eventRange.first;
        size_t remaining_chunk = m_events_per_chunk - total_events_to_read;

        // If the range size is larger than the remaining chunk, we need to split it
        if (range_size > remaining_chunk) {
          // Split the range: process only part of it now, push the rest back for later
          relative_target_ranges.emplace_back(target,
                                              EventROI(total_events_to_read, total_events_to_read + remaining_chunk));
          offsets.push_back(eventRange.first);
          slabsizes.push_back(remaining_chunk);
          total_events_to_read += remaining_chunk;
          // Push the remainder of the range back to the front for next iteration
          eventSplitRanges.emplace(target, EventROI(eventRange.first + remaining_chunk, eventRange.second));
          break;
        } else {
          relative_target_ranges.emplace_back(target,
                                              EventROI(total_events_to_read, total_events_to_read + range_size));
          offsets.push_back(eventRange.first);
          slabsizes.push_back(range_size);
          total_events_to_read += range_size;
          // Continue to next range
        }
      }

      // log the event ranges being processed
      g_log.debug(toLogString(bankName, total_events_to_read, offsets, slabsizes));

      if (total_events_to_read == 0) {
        continue; // nothing to do
      }

      // load detid and tof at the same time
      this->loadEvents(detID_SDS, tof_SDS, offsets, slabsizes, event_detid, event_time_of_flight);

      // loop over targets
      tbb::parallel_for(
          tbb::blocked_range<size_t>(0, m_workspaceIndices.size()), [&](const tbb::blocked_range<size_t> &r) {
            for (size_t idx = r.begin(); idx != r.end(); ++idx) {
              int i = m_workspaceIndices[idx];

              // Precompute indices for this target
              std::vector<size_t> indices;
              for (const auto &pair : relative_target_ranges) {
                if (pair.first == static_cast<int>(i)) {
                  auto [start, end] = pair.second;
                  for (size_t k = start; k < end; ++k) {
                    indices.push_back(k);
                  }
                }
              }

              auto event_id_view_for_target =
                  indices | std::views::transform([&event_detid](const auto &k) { return (*event_detid)[k]; });
              auto event_tof_view_for_target = indices | std::views::transform([&event_time_of_flight](const auto &k) {
                                                 return (*event_time_of_flight)[k];
                                               });

              // Loop over all output spectra / groups
              tbb::parallel_for(tbb::blocked_range<size_t>(0, m_processingDatas[i].counts.size()),
                                [&](const tbb::blocked_range<size_t> &output_range) {
                                  for (size_t output_index = output_range.begin(); output_index < output_range.end();
                                       ++output_index) {
                                    ProcessEventsTask task(&event_id_view_for_target, &event_tof_view_for_target,
                                                           &calibrations.at(output_index),
                                                           m_processingDatas[i].binedges[output_index]);

                                    const tbb::blocked_range<size_t> range_info(0, indices.size(), m_grainsize_event);
                                    tbb::parallel_reduce(range_info, task);

                                    // Accumulate results into shared y_temp to combine local histograms
                                    // Use atomic fetch_add to accumulate results into shared vectors
                                    for (size_t j = 0; j < m_processingDatas[i].counts[output_index].size(); ++j) {
                                      m_processingDatas[i].counts[output_index][j].fetch_add(task.y_temp[j],
                                                                                             std::memory_order_relaxed);
                                    }
                                  }
                                });
            }
          });
    }
    g_log.debug() << bankName << " stop " << timer << std::endl;
    m_progress->report();
  }
}
}; // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
