#include "MantidHistogramData/HistogramItem.h"
#include "MantidHistogramData/Histogram.h"
#include <algorithm>
#include <utility>

using Mantid::HistogramData::HistogramItem;
using Mantid::HistogramData::Histogram;
using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::Points;

HistogramItem::HistogramItem(const Histogram &histogram, const size_t index)
    : m_histogram(histogram), m_index(index) {}

double HistogramItem::center() const {
  const auto &x = m_histogram.x();
  if (xModeIsPoints()) {
    return x[m_index];
  } else {
    return (x[m_index + 1] + x[m_index]) / 2.0;
  }
}

double HistogramItem::binWidth() const {
  const auto &x = m_histogram.x();
  if (xModeIsPoints()) {
    auto numPoints = m_histogram.size();
    double lower = 0;
    double upper = 0;
    if (m_index == 0) {
      // first point
      return x[1] - x[0];
    } else if (m_index == numPoints - 1) {
      // last point
      return x[m_index] - x[m_index - 1];
    } else {
      // everything inbetween
      return 0.5 * (x[m_index + 1] - x[m_index - 1]);
    }
    return upper - lower;
  } else {
    return x[m_index + 1] - x[m_index];
  }
}

bool HistogramItem::xModeIsPoints() const {
  return Histogram::XMode::Points == m_histogram.xMode();
}

bool HistogramItem::yModeIsCounts() const {
  return Histogram::YMode::Counts == m_histogram.yMode();
}

double HistogramItem::counts() const {
  const auto &y = m_histogram.y();
  if (yModeIsCounts()) {
    return y[m_index];
  } else {
    return y[m_index] / binWidth();
  }
}

double HistogramItem::countVariance() const {
  const auto &e = m_histogram.e();
  if (yModeIsCounts()) {
    return e[m_index] * e[m_index];
  } else {
    const auto width = binWidth();
    return e[m_index] * e[m_index] * width * width;
  }
}

double HistogramItem::countStandardDeviation() const {
  const auto &e = m_histogram.e();
  if (yModeIsCounts()) {
    return e[m_index];
  } else {
    const auto width = binWidth();
    return e[m_index] * width;
  }
}

double HistogramItem::frequency() const {
  const auto &y = m_histogram.y();
  if (yModeIsCounts()) {
    return y[m_index] * binWidth();
  } else {
    return y[m_index];
  }
}

double HistogramItem::frequencyVariance() const {
  const auto &e = m_histogram.e();
  if (!yModeIsCounts()) {
    return e[m_index] * e[m_index];
  } else {
    const auto width = binWidth();
    return (e[m_index] * e[m_index]) / (width * width);
  }
}

double HistogramItem::frequencyStandardDeviation() const {
  const auto &e = m_histogram.e();
  if (!yModeIsCounts()) {
    return e[m_index];
  } else {
    const auto width = binWidth();
    return e[m_index] / width;
  }
}

void HistogramItem::advance(int64_t delta) {
  m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
                                 static_cast<uint64_t>(m_index) + delta)
                      : std::min(m_histogram.size(),
                                 m_index + static_cast<size_t>(delta));
}

void HistogramItem::incrementIndex() {
  if (m_index < m_histogram.size()) {
    ++m_index;
  }
}
