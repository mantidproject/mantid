// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/BankPulseTimes.h"
#include "MantidDataHandling/CompressEventAccumulator.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Task.h"

#include <vector>

namespace Mantid {
namespace API {
class Progress; // forward declare
}
namespace DataHandling {
class DefaultEventLoader; // forward declare

/** ProcessBankCompressed : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL ProcessBankCompressed : public Mantid::Kernel::Task {
public:
  ProcessBankCompressed(DefaultEventLoader &m_loader, const std::string &entry_name, Mantid::API::Progress *prog,
                        std::shared_ptr<std::vector<uint32_t>> event_detid,
                        std::shared_ptr<std::vector<float>> event_tof, size_t startAt,
                        std::shared_ptr<std::vector<uint64_t>> event_index,
                        std::shared_ptr<BankPulseTimes> bankPulseTimes, detid_t min_detid, detid_t max_detid,
                        std::shared_ptr<std::vector<double>> histogram_bin_edges, const double divisor);

  void run() override;

  void addEvent(const size_t period_index, const detid_t detid, const float tof);

  void createWeightedEvents(const size_t period_index, const detid_t detid,
                            std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events);

  /// method only intended for testing
  double totalWeight() const;

private:
  void collectEvents();
  void addToEventLists();

  // disable default constructor
  ProcessBankCompressed();
  /// Algorithm being run
  DefaultEventLoader &m_loader;
  const std::string m_entry_name;
  /// Progress reporting
  API::Progress *m_prog;

  /// event pixel ID array
  std::shared_ptr<std::vector<uint32_t>> m_event_detid;
  /// event TOF array
  std::shared_ptr<std::vector<float>> m_event_tof;
  /// index of the first event from event_index
  const size_t m_firstEventIndex;
  /// vector of event index (length of # of pulses)
  std::shared_ptr<std::vector<uint64_t>> m_event_index;
  /// Pulse times for this bank
  std::shared_ptr<BankPulseTimes> m_bankPulseTimes;

  /**
   * Objects holding individual spectra. This is accessed as [periodIndex][detidIndex]
   */
  std::vector<std::vector<std::unique_ptr<DataHandling::CompressEventAccumulator>>> m_spectra_accum;

  // inclusive
  const detid_t m_detid_min;
  // inclusive
  const detid_t m_detid_max;
  // inclusive
  const float m_tof_min;
  // exclusive
  const float m_tof_max;
};

} // namespace DataHandling
} // namespace Mantid
