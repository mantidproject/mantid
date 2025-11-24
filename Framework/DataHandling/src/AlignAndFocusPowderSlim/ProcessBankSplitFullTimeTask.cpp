// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankSplitFullTimeTask.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessEventsTask.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ParallelMinMax.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/Unit.h"
#include "MantidNexus/H5Util.h"
#include "tbb/parallel_for.h"
#include "tbb/parallel_reduce.h"

#include <ranges>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

namespace {

const std::string MICROSEC("microseconds");

// Logger for this class
auto g_log = Kernel::Logger("ProcessBankSplitFullTimeTask");

} // namespace
ProcessBankSplitFullTimeTask::ProcessBankSplitFullTimeTask(
    std::vector<std::string> &bankEntryNames, H5::H5File &h5file, const bool is_time_filtered,
    std::vector<int> &workspaceIndices, std::vector<API::MatrixWorkspace_sptr> &wksps,
    const std::map<detid_t, double> &calibration, const std::map<detid_t, double> &scale_at_sample,
    const std::set<detid_t> &masked, const size_t events_per_chunk, const size_t grainsize_event,
    const std::vector<PulseROI> &pulse_indices, const std::map<Mantid::Types::Core::DateAndTime, int> &splitterMap,
    const bool correction_to_sample, std::shared_ptr<API::Progress> &progress)
    : m_h5file(h5file), m_bankEntries(bankEntryNames),
      m_loader(std::make_shared<NexusLoader>(is_time_filtered, pulse_indices)), m_workspaceIndices(workspaceIndices),
      m_wksps(wksps), m_calibration(calibration), m_scale_at_sample(scale_at_sample), m_masked(masked),
      m_events_per_chunk(events_per_chunk), m_splitterMap(splitterMap), m_grainsize_event(grainsize_event),
      m_correction_to_sample(correction_to_sample), m_progress(progress) {}

ProcessBankSplitFullTimeTask::ProcessBankSplitFullTimeTask(
    std::vector<std::string> &bankEntryNames, H5::H5File &h5file, std::shared_ptr<NexusLoader> loader,
    std::vector<int> &workspaceIndices, std::vector<API::MatrixWorkspace_sptr> &wksps,
    const std::map<detid_t, double> &calibration, const std::map<detid_t, double> &scale_at_sample,
    const std::set<detid_t> &masked, const size_t events_per_chunk, const size_t grainsize_event,
    const std::map<Mantid::Types::Core::DateAndTime, int> &splitterMap, const bool correction_to_sample,
    std::shared_ptr<API::Progress> &progress)
    : m_h5file(h5file), m_bankEntries(bankEntryNames), m_loader(loader), m_workspaceIndices(workspaceIndices),
      m_wksps(wksps), m_calibration(calibration), m_scale_at_sample(scale_at_sample), m_masked(masked),
      m_events_per_chunk(events_per_chunk), m_splitterMap(splitterMap), m_grainsize_event(grainsize_event),
      m_correction_to_sample(correction_to_sample), m_progress(progress) {}

void ProcessBankSplitFullTimeTask::operator()(const tbb::blocked_range<size_t> &range) const {
  auto entry = m_h5file.openGroup("entry"); // type=NXentry
  for (size_t wksp_index = range.begin(); wksp_index < range.end(); ++wksp_index) {
    const auto &bankName = m_bankEntries[wksp_index];
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

    std::unique_ptr<std::vector<uint64_t>> event_index = std::make_unique<std::vector<uint64_t>>();
    auto eventRanges = m_loader->getEventIndexRanges(event_group, total_events, &event_index);

    // Get all spectra for this bank.
    // Create temporary y arrays for each workspace.
    std::vector<API::ISpectrum *> spectra;
    std::vector<std::vector<uint32_t>> y_temps;
    for (const auto &wksp : m_wksps) {
      spectra.push_back(&wksp->getSpectrum(wksp_index));
      y_temps.emplace_back(spectra.back()->dataY().size());
    }

    // create object so bank calibration can be re-used
    std::unique_ptr<BankCalibration> calibration = nullptr;

    // get handle to the data
    auto detID_SDS = event_group.openDataSet(NxsFieldNames::DETID);
    std::string tof_unit;
    Nexus::H5Util::readStringAttribute(tof_SDS, "units", tof_unit);
    const double time_conversion = Kernel::Units::timeConversionValue(tof_unit, MICROSEC);

    const auto frequency_log =
        dynamic_cast<const Kernel::TimeSeriesProperty<double> *>(m_wksps.at(0)->run().getProperty("frequency"));
    const auto pulse_times =
        std::make_unique<std::vector<Mantid::Types::Core::DateAndTime>>(frequency_log->timesAsVector());

    // declare arrays once so memory can be reused
    auto event_detid = std::make_unique<std::vector<uint32_t>>();       // uint32 for ORNL nexus file
    auto event_time_of_flight = std::make_unique<std::vector<float>>(); // float for ORNL nexus files
    auto pulse_times_idx = std::make_unique<std::vector<size_t>>();     // index into pulse_times for every event

    // read parts of the bank at a time until all events are processed
    while (!eventRanges.empty()) {
      // Create offsets and slab sizes for the next chunk of events.
      // This will read at most m_events_per_chunk events from the file
      // and will split the ranges if necessary for the next iteration.
      std::vector<size_t> offsets;
      std::vector<size_t> slabsizes;

      size_t total_events_to_read = 0;
      // Process the event ranges until we reach the desired number of events to read or run out of ranges
      while (!eventRanges.empty() && total_events_to_read < m_events_per_chunk) {
        // Get the next event range from the stack
        auto eventRange = eventRanges.top();
        eventRanges.pop();

        size_t range_size = eventRange.second - eventRange.first;
        size_t remaining_chunk = m_events_per_chunk - total_events_to_read;

        // If the range size is larger than the remaining chunk, we need to split it
        if (range_size > remaining_chunk) {
          // Split the range: process only part of it now, push the rest back for later
          offsets.push_back(eventRange.first);
          slabsizes.push_back(remaining_chunk);
          total_events_to_read += remaining_chunk;
          // Push the remainder of the range back to the front for next iteration
          eventRanges.emplace(eventRange.first + remaining_chunk, eventRange.second);
          break;
        } else {
          offsets.push_back(eventRange.first);
          slabsizes.push_back(range_size);
          total_events_to_read += range_size;
          // Continue to next range
        }
      }

      // log the event ranges being processed
      std::ostringstream oss;
      oss << "Processing " << bankName << " with " << total_events_to_read << " events in the ranges: ";
      for (size_t i = 0; i < offsets.size(); ++i) {
        oss << "[" << offsets[i] << ", " << (offsets[i] + slabsizes[i]) << "), ";
      }
      oss << "\n";
      g_log.debug() << oss.str();

      // load detid and tof at the same time
      tbb::parallel_invoke(
          [&] { // load detid
            m_loader->loadData(detID_SDS, event_detid, offsets, slabsizes);
            // immediately find min/max to allow for other things to read disk
            const auto [minval, maxval] = Mantid::Kernel::parallel_minmax(event_detid, m_grainsize_event);
            // only recreate calibration if it doesn't already have the useful information
            if ((!calibration) || (calibration->idmin() > static_cast<detid_t>(minval)) ||
                (calibration->idmax() < static_cast<detid_t>(maxval))) {
              calibration =
                  std::make_unique<BankCalibration>(static_cast<detid_t>(minval), static_cast<detid_t>(maxval),
                                                    time_conversion, m_calibration, m_scale_at_sample, m_masked);
            }
          },
          [&] { // load time-of-flight
            m_loader->loadData(tof_SDS, event_time_of_flight, offsets, slabsizes);
          });

      pulse_times_idx->resize(total_events_to_read);
      // get the pulsetime of every event, event_index maps the first event of each pulse
      size_t pos = 0;
      auto event_index_it = event_index->cbegin();
      for (size_t i = 0; i < offsets.size(); ++i) {
        const size_t off = offsets[i];
        const size_t slab = slabsizes[i];
        for (size_t j = 0; j < slab; ++j) {
          const uint64_t global_idx = static_cast<uint64_t>(off + j);
          // find pulse: event_index holds the first event index of each pulse
          while (event_index_it != event_index->cend() && *event_index_it <= global_idx) {
            ++event_index_it;
          }
          size_t pulse_idx = 0;
          if (event_index_it != event_index->cbegin()) {
            pulse_idx = static_cast<size_t>(std::distance(event_index->cbegin(), event_index_it) - 1);
          }
          (*pulse_times_idx)[pos++] = pulse_idx;
        }
      }

      // loop over targets
      tbb::parallel_for(
          tbb::blocked_range<size_t>(0, m_workspaceIndices.size()), [&](const tbb::blocked_range<size_t> &r) {
            for (size_t idx = r.begin(); idx != r.end(); ++idx) {
              int i = m_workspaceIndices[idx];

              // Precompute indices for this target
              std::vector<size_t> indices;
              auto splitter_it = m_splitterMap.cbegin();
              for (size_t k = 0; k < event_detid->size(); ++k) {
                // Calculate the full time at sample: full_time = pulse_time + (tof * correctionFactor), where
                // correctionFactor is either scale_at_sample[detid] or 1.0
                const double correctionFactor =
                    m_correction_to_sample ? calibration->value_scale_at_sample(static_cast<detid_t>((*event_detid)[k]))
                                           : 1.0;

                const auto tof = static_cast<double>((*event_time_of_flight)[k]) * correctionFactor;
                const auto tof_in_nanoseconds =
                    static_cast<int64_t>(tof * 1000.0); // Convert microseconds to nanoseconds
                const auto pulsetime = (*pulse_times)[(*pulse_times_idx)[k]];
                const Mantid::Types::Core::DateAndTime full_time = pulsetime + tof_in_nanoseconds;
                // Linear search for pulsetime in splitter map, assume pulsetime and splitter map are both sorted. This
                // is the starting point for the full_time search so we need to subtract some time (66.6ms) to ensure we
                // don't skip it when adding the tof.
                // Advance splitter_it until it points to the first element greater than (pulsetime - offset).
                // This gives us a reasonable starting point for the full_time search.
                while (splitter_it != m_splitterMap.end() &&
                       splitter_it->first <= pulsetime - static_cast<int64_t>(PULSETIME_OFFSET)) {
                  ++splitter_it;
                }

                // Now, starting at splitter_it, find full_time (TOFs will be unsorted)
                // Advance until we find the first element > full_time, then step back to get the greatest key <=
                // full_time.
                auto full_time_it = splitter_it;
                while (full_time_it != m_splitterMap.end() && full_time_it->first <= full_time) {
                  ++full_time_it;
                }

                // If there is no element <= full_time then full_time_it will be begin(); otherwise step back to the
                // element that is <= full_time.
                if (full_time_it == m_splitterMap.begin()) {
                  // no splitter entry <= full_time; skip this event
                } else {
                  --full_time_it;
                  if (full_time_it != m_splitterMap.end() && full_time_it->second == i) {
                    indices.push_back(k);
                  }
                }
              }

              auto event_id_view_for_target =
                  indices | std::views::transform([&event_detid](const auto &k) { return (*event_detid)[k]; });
              auto event_tof_view_for_target = indices | std::views::transform([&event_time_of_flight](const auto &k) {
                                                 return (*event_time_of_flight)[k];
                                               });

              ProcessEventsTask task(&event_id_view_for_target, &event_tof_view_for_target, calibration.get(),
                                     &spectra[idx]->readX());

              const tbb::blocked_range<size_t> range_info(0, indices.size(), m_grainsize_event);
              tbb::parallel_reduce(range_info, task);

              std::transform(y_temps[idx].begin(), y_temps[idx].end(), task.y_temp.begin(), y_temps[idx].begin(),
                             std::plus<uint32_t>());
            }
          });
    }

    // copy the data out into the correct spectrum
    tbb::parallel_for(size_t(0), m_wksps.size(), [&](size_t i) {
      auto &y_values = spectra[i]->dataY();
      std::copy(y_temps[i].cbegin(), y_temps[i].cend(), y_values.begin());
    });

    g_log.debug() << bankName << " stop" << timer << std::endl;
    m_progress->report();
  }
}
}; // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
