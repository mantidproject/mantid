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

double HistogramItem::counts() const {
  const auto &yMode = histogramRef().yMode();
  if (yMode == Histogram::YMode::Counts) {
    return histogramRef().counts()[m_index];
  } else {
    const Frequencies frequency{histogramRef().frequencies()[m_index]};
    return Counts(frequency, binEdges())[0];
  }
}

double HistogramItem::countVariance() const {
  return histogramRef().countVariances()[m_index];
}

double HistogramItem::countStandardDeviation() const {
  return histogramRef().countStandardDeviations()[m_index];
}

double HistogramItem::frequency() const {
  const auto &yMode = histogramRef().yMode();
  if (yMode == Histogram::YMode::Frequencies) {
    return histogramRef().frequencies()[m_index];
  } else {
    const Counts counts{histogramRef().counts()[m_index]};
    return Frequencies(counts, binEdges())[0];
  }
}

double HistogramItem::frequencyVariance() const {
  return histogramRef().frequencyVariances()[m_index];
}

double HistogramItem::frequencyStandardDeviation() const {
  return histogramRef().frequencyStandardDeviations()[m_index];
}

double HistogramItem::center() const { return point()[0]; }

double HistogramItem::width() const {
  const auto &edges = binEdges();
  const auto &lower = edges[0];
  const auto &upper = edges[1];
  return upper - lower;
}

const BinEdges HistogramItem::binEdges() const {
  const auto &xMode = histogramRef().xMode();

  if (xMode == Histogram::XMode::BinEdges) {
    const auto &edges = histogramRef().binEdges();
    const auto &lower = edges[m_index];
    const auto &upper = edges[m_index + 1];
    return BinEdges{lower, upper};
  } else {
    const auto &points = histogramRef().points();
    // Guess the bin edges from the points
    if (points.size() == 0) {
      // We have no points, just return empty bin edges
      return BinEdges{};
    } else if (points.size() == 1) {
      // We have a single point, let BinEdges guess the edges
      return BinEdges(points);
    } else {
      // We have 2 or more points. Attempt to use up to 4 points
      // to interpolate the correct bin edges.
      size_t upperBound = std::min(m_index + 2, points.size());
      size_t lowerBound = (m_index < 2) ? 0 : m_index - 2;

      std::vector<double> pts;
      for (size_t i = lowerBound; i < upperBound; i++) {
        pts.push_back(points[i]);
      }

      auto index = m_index - lowerBound;
      BinEdges edges(Points{pts});
      return BinEdges{edges[index], edges[index + 1]};
    }
  }
}

const Points HistogramItem::point() const {
  const auto &xMode = histogramRef().xMode();
  if (xMode == Histogram::XMode::Points) {
    return Points{histogramRef().points()[m_index]};
  } else {
    const auto &lower = histogramRef().binEdges()[m_index];
    const auto &upper = histogramRef().binEdges()[m_index + 1];
    const BinEdges bins{lower, upper};
    return Points(bins);
  }
}

const Histogram &HistogramItem::histogramRef() const { return m_histogram; }
