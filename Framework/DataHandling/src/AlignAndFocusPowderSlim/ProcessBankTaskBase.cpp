// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/AlignAndFocusPowderSlim/ProcessBankTaskBase.h"
#include "MantidKernel/Unit.h"
#include <sstream>
#include <tbb/tbb.h>

namespace {
const std::string MICROSEC("microseconds");
}

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {
ProcessBankTaskBase::ProcessBankTaskBase(std::vector<std::string> &bankEntryNames, std::shared_ptr<NexusLoader> loader,
                                         const BankCalibrationFactory &calibFactory)
    : m_bankEntries(bankEntryNames), m_loader(std::move(loader)), m_calibFactory(calibFactory) {}

const std::string &ProcessBankTaskBase::bankName(const size_t wksp_index) const { return m_bankEntries[wksp_index]; }

std::vector<BankCalibration> ProcessBankTaskBase::getCalibrations(const std::string &tof_unit,
                                                                  const size_t bank_index) const {
  // when arbitrary grouping is used, we need all calibrations
  const double time_conversion = Kernel::Units::timeConversionValue(tof_unit, MICROSEC);
  return m_calibFactory.getCalibrations(time_conversion, bank_index);
}

void ProcessBankTaskBase::loadEvents(H5::DataSet &detID_SDS, H5::DataSet &tof_SDS, const std::vector<size_t> &offsets,
                                     const std::vector<size_t> &slabsizes,
                                     std::unique_ptr<std::vector<uint32_t>> &detId_vec,
                                     std::unique_ptr<std::vector<float>> &tof_vec) const {
  tbb::parallel_invoke(
      [&] { // load detid
        m_loader->loadData(detID_SDS, detId_vec, offsets, slabsizes);
      },
      [&] { // load time-of-flight
        m_loader->loadData(tof_SDS, tof_vec, offsets, slabsizes);
      });
}

std::stack<EventROI>
ProcessBankTaskBase::getEventIndexRanges(H5::Group &event_group, const uint64_t number_events,
                                         std::unique_ptr<std::vector<uint64_t>> *event_index) const {
  return m_loader->getEventIndexRanges(event_group, number_events, event_index);
}

std::stack<std::pair<int, EventROI>> ProcessBankTaskBase::getEventIndexSplitRanges(H5::Group &event_group,
                                                                                   const uint64_t number_events) const {
  return m_loader->getEventIndexSplitRanges(event_group, number_events);
}

std::string toLogString(const std::string &bankName, const size_t total_events_to_read,
                        const std::vector<size_t> &offsets, const std::vector<size_t> &slabsizes) {
  std::ostringstream oss;
  oss << "Processing " << bankName << " with " << total_events_to_read << " events in the ranges: ";
  for (size_t i = 0; i < offsets.size(); ++i) {
    oss << "[" << offsets[i] << ", " << (offsets[i] + slabsizes[i]) << "), ";
  }
  oss << "\n";

  return oss.str();
}

}; // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
