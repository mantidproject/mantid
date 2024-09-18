// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <utility>

#include "MantidDataHandling/DataBlock.h"
#include "MantidDataHandling/DataBlockGenerator.h"

namespace Mantid::DataHandling {

// -------------------------------------------------------------
// DataBlock Generator
// -------------------------------------------------------------
DataBlockGenerator::DataBlockGenerator(std::vector<SpectrumPair> intervals) : m_intervals(std::move(intervals)) {
  // We need to sort the data items.
  auto comparison = [](const SpectrumPair &el1, const SpectrumPair &el2) { return el1.first < el2.first; };
  std::sort(m_intervals.begin(), m_intervals.end(), comparison);

  // If there is an interval then set the current index to the first interval in
  // the sorted container
  if (!m_intervals.empty()) {
    m_currentIntervalIndex = 0;
    m_currentSpectrum = m_intervals[m_currentIntervalIndex.value()].first;

  } else {
    m_currentIntervalIndex = std::nullopt;
    m_currentSpectrum = -1;
  }
}

/**
 * We need to increment through a series of intervals and need to
 * make sure that we skip the gaps. This does assume that the intervals
 * are ordered.
 */
DataBlockGenerator &DataBlockGenerator::operator++() {
  ++m_currentSpectrum;

  // If there is no valid interval index then do nothing
  if (m_currentIntervalIndex) {
    // We need to check if this index is still in the current interval
    // If not we need to increment the interval or set the interval index
    // to a final state
    auto isinCurrentInterval = m_intervals[m_currentIntervalIndex.value()].first <= m_currentSpectrum &&
                               m_currentSpectrum <= m_intervals[m_currentIntervalIndex.value()].second;

    if (!isinCurrentInterval) {
      ++(*m_currentIntervalIndex);

      // Check if we are past the last interval or else set it to the
      // first element of the new interval
      if (m_currentIntervalIndex.value() > (m_intervals.size() - 1)) {
        m_currentIntervalIndex = std::nullopt;
      } else {
        m_currentSpectrum = m_intervals[m_currentIntervalIndex.value()].first;
      }
    }
  }

  return *this;
}

// Postincrement version
DataBlockGenerator DataBlockGenerator::operator++(int) {
  DataBlockGenerator temp(this->m_intervals);
  ++(*this);
  return temp;
}

/**
 * Convenience method for incrementing
 */
void DataBlockGenerator::next() { ++(*this); }

bool DataBlockGenerator::isDone() { return !m_currentIntervalIndex; }

specnum_t DataBlockGenerator::getValue() { return m_currentSpectrum; }
} // namespace Mantid::DataHandling
