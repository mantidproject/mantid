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
  const auto &x = histogramRef().x();
  if (xModeIsPoints()) {
    return x[m_index];
  } else {
    return (x[m_index + 1] + x[m_index]) / 2.0;
  }
}

double HistogramItem::width() const {
  const auto &x = histogramRef().x();
  if (xModeIsPoints()) {
    auto numPoints = histogramRef().size();
    double lower = 0;
    double upper = 0;
    if (m_index == 0) {
      // first point
      upper = 0.5 * (x[m_index + 1] + x[m_index]);
      lower = x[0] - (upper - x[0]);
    } else if (m_index == numPoints - 1) {
      // last point
      lower = 0.5 * (x[m_index] + x[m_index - 1]);
      upper = x[numPoints - 1] + (x[numPoints - 1] - lower);
    } else {
      // everything inbetween
      lower = 0.5 * (x[m_index] + x[m_index - 1]);
      upper = 0.5 * (x[m_index + 1] + x[m_index]);
    }
    return upper - lower;
  } else {
    return x[m_index + 1] - x[m_index];
  }
}

bool HistogramItem::xModeIsPoints() const {
  return Histogram::XMode::Points == histogramRef().xMode();
}

bool HistogramItem::yModeIsCounts() const {
  return Histogram::YMode::Counts == histogramRef().yMode();
}

double HistogramItem::counts() const {
  const auto &y = histogramRef().y();
  if (yModeIsCounts()) {
    return y[m_index];
  } else {
    return y[m_index] / width();
  }
}

double HistogramItem::countVariance() const {
  const auto &e = histogramRef().e();
  if (yModeIsCounts()) {
    return e[m_index] * e[m_index];
  } else {
    const auto binWidth = width();
    return e[m_index] * e[m_index] * binWidth * binWidth;
  }
}

double HistogramItem::countStandardDeviation() const {
  const auto &e = histogramRef().e();
  if (yModeIsCounts()) {
    return e[m_index];
  } else {
    const auto binWidth = width();
    return e[m_index] * binWidth;
  }
}

double HistogramItem::frequency() const {
  const auto &y = histogramRef().y();
  if (yModeIsCounts()) {
    return y[m_index] * width();
  } else {
    return y[m_index];
  }
}

double HistogramItem::frequencyVariance() const {
  const auto &e = histogramRef().e();
  if (!yModeIsCounts()) {
    return e[m_index] * e[m_index];
  } else {
    const auto binWidth = width();
    return (e[m_index] * e[m_index]) / (binWidth * binWidth);
  }
}

double HistogramItem::frequencyStandardDeviation() const {
  const auto &e = histogramRef().e();
  if (!yModeIsCounts()) {
    return e[m_index];
  } else {
    const auto binWidth = width();
    return e[m_index] / binWidth;
  }
}

const Histogram &HistogramItem::histogramRef() const { return m_histogram; }

void HistogramItem::advance(int64_t delta) {
  m_index = delta < 0 ? std::max(static_cast<uint64_t>(0),
                                 static_cast<uint64_t>(m_index) + delta)
                      : std::min(histogramRef().size(),
                                 m_index + static_cast<size_t>(delta));
}

void HistogramItem::incrementIndex() {
  if (m_index < histogramRef().size()) {
    ++m_index;
  }
}
