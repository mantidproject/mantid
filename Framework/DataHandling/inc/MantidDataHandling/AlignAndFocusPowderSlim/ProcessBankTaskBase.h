// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once
#include "MantidAPI/ISpectrum.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/BankCalibration.h"
#include "MantidDataHandling/AlignAndFocusPowderSlim/NexusLoader.h"
#include <H5Cpp.h>

namespace Mantid::DataHandling::AlignAndFocusPowderSlim {

class ProcessBankTaskBase {
public:
  ProcessBankTaskBase(std::vector<std::string> &bankEntryNames, std::shared_ptr<NexusLoader> loader,
                      const BankCalibrationFactory &calibFactory);
  const std::string &bankName(const size_t wksp_index) const;
  std::vector<BankCalibration> getCalibrations(const std::string &tof_unit, const size_t bank_index) const;

  /**
   *  Load detid and tof at the same time
   *
   *  @param detID_SDS : HDF5 dataset for detector IDs
   *  @param tof_SDS : HDF5 dataset for time-of-flights
   *  @param offsets : offsets to read from each dataset
   *  @param slabsizes : slab sizes to read from each dataset
   *  @param detId_vec : output vector for detector IDs
   *  @param tof_vec : output vector for time-of-flights
   */
  void loadEvents(H5::DataSet &detID_SDS, H5::DataSet &tof_SDS, const std::vector<size_t> &offsets,
                  const std::vector<size_t> &slabsizes, std::unique_ptr<std::vector<uint32_t>> &detId_vec,
                  std::unique_ptr<std::vector<float>> &tof_vec) const;

  // these methods simply forward the calls to NexusLoader
  std::stack<EventROI> getEventIndexRanges(H5::Group &event_group, const uint64_t number_events,
                                           std::unique_ptr<std::vector<uint64_t>> *event_index = nullptr) const;
  std::stack<std::pair<int, EventROI>> getEventIndexSplitRanges(H5::Group &event_group,
                                                                const uint64_t number_events) const;

private:
  const std::vector<std::string> m_bankEntries;
  std::shared_ptr<const NexusLoader> m_loader;
  /// used to generate actual calibration
  const BankCalibrationFactory &m_calibFactory;
};

std::string toLogString(const std::string &bankName, const size_t total_events_to_read,
                        const std::vector<size_t> &offsets, const std::vector<size_t> &slabsizes);
} // namespace Mantid::DataHandling::AlignAndFocusPowderSlim
