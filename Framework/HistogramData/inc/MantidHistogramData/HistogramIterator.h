// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramItem.h"

#include <boost/iterator/iterator_facade.hpp>
#include <memory>

namespace Mantid {
namespace HistogramData {

class Histogram;

/** HistogramIterator

  HistogramIterator implements an iterator interface for HistogramData.
  At each position the iterator will point to an instance of a HistogramItem.
  This item provides direct access to the values at a particular index.

  @author Samuel Jackson
  @date 2017
*/
class MANTID_HISTOGRAMDATA_DLL HistogramIterator
    : public boost::iterator_facade<HistogramIterator, const HistogramItem &, boost::bidirectional_traversal_tag> {

public:
  HistogramIterator(const Histogram &histogram, const size_t index) : m_item(histogram, index) {};

private:
  friend class boost::iterator_core_access;

  void advance(int64_t delta) {
    m_item.m_index = delta < 0 ? std::max(static_cast<uint64_t>(0), static_cast<uint64_t>(m_item.m_index) + delta)
                               : std::min(m_item.m_histogram.size(), m_item.m_index + static_cast<size_t>(delta));
  }

  void increment() {
    if (m_item.m_index < m_item.m_histogram.size()) {
      ++m_item.m_index;
    }
  }

  void decrement() {
    if (m_item.m_index > 0) {
      --m_item.m_index;
    }
  }

  size_t getIndex() const { return m_item.m_index; }

  void setIndex(const size_t index) { m_item.m_index = index; }

  bool equal(const HistogramIterator &other) const { return getIndex() == other.getIndex(); }

  const HistogramItem &dereference() const { return m_item; }

  uint64_t distance_to(const HistogramIterator &other) const {
    return static_cast<uint64_t>(other.getIndex()) - static_cast<uint64_t>(getIndex());
  }

  HistogramItem m_item;
};

} // namespace HistogramData
} // namespace Mantid
