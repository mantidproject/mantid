#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/HistogramIterator.h"
#include "MantidKernel/make_unique.h"

using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::HistogramIterator;
using Mantid::Kernel::make_unique;

void HistogramIterator::increment() {
    if (m_index < m_histogram.size()) {
        ++m_index;
        m_currentItem = make_unique<HistogramItem>(m_histogram, m_index); 
    }
}

bool HistogramIterator::equal(const HistogramIterator &other) const {
    return (&m_histogram == &other.m_histogram) && (m_index == other.m_index);
}

HistogramItem& HistogramIterator::dereference() const { 
    return *m_currentItem;
}

void HistogramIterator::decrement() {
    if (m_index > 0) {
        --m_index;
        m_currentItem = make_unique<HistogramItem>(m_histogram, m_index); 
    }
}

void HistogramIterator::advance(uint64_t delta) {
    m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
            static_cast<uint64_t>(m_index) + delta)
        : std::min(m_histogram.size(),
                m_index + static_cast<size_t>(delta));
    m_currentItem = make_unique<HistogramItem>(m_histogram, m_index); 
}
uint64_t HistogramIterator::distance_to(const HistogramIterator &other) const {
    return static_cast<uint64_t>(other.m_index) -
        static_cast<uint64_t>(m_index);
}
