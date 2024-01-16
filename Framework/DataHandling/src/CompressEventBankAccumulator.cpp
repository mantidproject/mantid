// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataHandling/CompressEventBankAccumulator.h"

#include <iostream>

namespace Mantid {
namespace DataHandling {
CompressEventBankAccumulator::CompressEventBankAccumulator(detid_t min_detid, detid_t max_detid,
                                                           std::shared_ptr<std::vector<double>> histogram_bin_edges,
                                                           const double divisor)
    : m_detid_min(min_detid), m_detid_max(max_detid), m_tof_min(static_cast<float>(histogram_bin_edges->front())),
      m_tof_max(static_cast<float>(histogram_bin_edges->back())) {
  const auto num_dets = static_cast<size_t>(m_detid_max - m_detid_min) + 1;
  const auto bin_mode = (divisor >= 0) ? CompressBinningMode::LINEAR : CompressBinningMode::LOGARITHMIC;

  // create the specra accumulators
  m_spectra_accum.reserve(num_dets);
  for (size_t det_index = 0; det_index <= num_dets; ++det_index) {
    m_spectra_accum.emplace_back(histogram_bin_edges, abs(divisor), bin_mode);
  }
}

void CompressEventBankAccumulator::addEvent(const detid_t detid, const float tof) {
  // comparing to integers is cheapest
  if ((detid < m_detid_min) || detid > m_detid_max) {
    // std::cout << "Skipping detid: " << m_detid_min << " < " << detid << " < " << m_detid_max << "\n";
    return; // early
  }

  // check if the tof is within range
  const auto tof_f = static_cast<float>(tof);
  if (((tof_f - m_tof_min) * (tof_f - m_tof_max) > 0.)) {
    // std::cout << "Skipping tof: " << tof_f << "\n";
    return; // early
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[det_index].addEvent(tof_f);
}

void CompressEventBankAccumulator::createWeightedEvents(
    const detid_t detid, std::vector<Mantid::DataObjects::WeightedEventNoTime> *raw_events) const {
  if ((detid < m_detid_min) || detid > m_detid_max) {
    std::stringstream msg;
    msg << "Encountered invalid detid=" << detid;
    throw std::runtime_error(msg.str());
  }

  const auto det_index = static_cast<size_t>(detid - m_detid_min);
  m_spectra_accum[det_index].createWeightedEvents(raw_events);
}

std::size_t CompressEventBankAccumulator::numberWeightedEvents() const {
  size_t totalWeightedEvents =
      std::accumulate(m_spectra_accum.cbegin(), m_spectra_accum.cend(), static_cast<size_t>(0),
                      [](const auto &current, const auto &value) { return current + value.numberWeightedEvents(); });
  return totalWeightedEvents;
}

} // namespace DataHandling
} // namespace Mantid
