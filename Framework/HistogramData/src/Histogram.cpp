// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramIterator.h"

#include <sstream>

namespace Mantid::HistogramData {

/** Obtain a valid XMode for a Histogram based on x and y data size.

  Throws if there is no valid XMode, i.e., of x-length is neither equal to or 1
  larger than y-length. */
Histogram::XMode getHistogramXMode(size_t xLength, size_t yLength) {
  if (xLength == yLength)
    return Histogram::XMode::Points;
  if (xLength == (yLength + 1))
    return Histogram::XMode::BinEdges;
  throw std::logic_error("getHistogramXMode(): x-y size mismatch, cannot "
                         "determine Histogram::XMode.");
}

/** Returns the bin edges of the Histogram.

  If the histogram stores points, the bin edges are computed based on them.
  Otherwise the returned BinEdges' internal pointer references the same data as
  the Histogram, i.e., there is little overhead. */
BinEdges Histogram::binEdges() const {
  if (xMode() == XMode::BinEdges)
    return BinEdges(m_x);
  else
    return BinEdges(Points(m_x));
}

/** Returns the points (or bin centers) of the Histogram.

  If the histogram stores bin edges, the points are computed based on them.
  Otherwise the returned Points's internal pointer references the same data as
  the Histogram, i.e., there is little overhead. */
Points Histogram::points() const {
  if (xMode() == XMode::BinEdges)
    return Points(BinEdges(m_x));
  else
    return Points(m_x);
}

/// Returns the variances of the points (or bin centers) of the Histogram.
PointVariances Histogram::pointVariances() const {
  // Currently data is always stored as standard deviations, need to convert.
  return PointVariances(PointStandardDeviations(m_dx));
}

/// Returns the standard deviations of the points (or bin centers) of the
/// Histogram.
PointStandardDeviations Histogram::pointStandardDeviations() const { return PointStandardDeviations(m_dx); }

/** Returns the counts of the Histogram.

  If the histogram stores frequencies, the counts are computed based on them.
  Otherwise the returned Counts's internal pointer references the same data as
  the Histogram, i.e., there is little overhead. */
Counts Histogram::counts() const {
  if (yMode() != YMode::Frequencies)
    return Counts(m_y);
  else
    return Counts(Frequencies(m_y), binEdges());
}

/** Returns the variances of the counts of the Histogram.

  The variances are computed from the standard deviations that are stored in the
  Histogram, i.e., this method comes with an overhead. */
CountVariances Histogram::countVariances() const { return CountVariances(countStandardDeviations()); }

/** Returns the standard deviations of the counts of the Histogram.

  If the histogram stores frequencies, the CountStandardDeviations are computed
  based on the standard deviations of the frequencies. Otherwise the returned
  CountStandardDeviations's internal pointer references the same data as the
  Histogram, i.e., there is little overhead. */
CountStandardDeviations Histogram::countStandardDeviations() const {
  if (yMode() != YMode::Frequencies)
    return CountStandardDeviations(m_e);
  else
    return CountStandardDeviations(FrequencyStandardDeviations(m_e), binEdges());
}

/** Returns the frequencies of the Histogram, i.e., the counts divided by the
  bin widths.

  If the histogram stores counts, the frequencies are computed based on them.
  Otherwise the returned Frequencies's internal pointer references the same data
  as the Histogram, i.e., there is little overhead. */
Frequencies Histogram::frequencies() const {
  if (yMode() == YMode::Counts)
    return Frequencies(Counts(m_y), binEdges());
  else
    return Frequencies(m_y);
}

/** Returns the variances of the frequencies of the Histogram.

  The variances are computed on the fly from other data stored in the
  Histogram, i.e., this method comes with an overhead. */
FrequencyVariances Histogram::frequencyVariances() const { return FrequencyVariances(frequencyStandardDeviations()); }

/** Returns the standard deviations of the frequencies of the Histogram.

  If the histogram stores counts, the FrequencyStandardDeviations are computed
  based on the standard deviations of the counts. Otherwise the returned
  FrequencyStandardDeviations's internal pointer references the same data as the
  Histogram, i.e., there is little overhead. */
FrequencyStandardDeviations Histogram::frequencyStandardDeviations() const {
  if (yMode() == YMode::Counts)
    return FrequencyStandardDeviations(CountStandardDeviations(m_e), binEdges());
  else
    return FrequencyStandardDeviations(m_e);
}

/** Sets the internal x-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedX(const Kernel::cow_ptr<HistogramX> &x) & {
  if (m_x->size() != x->size())
    throw std::logic_error("Histogram::setSharedX: size mismatch\n");
  m_x = x;
}

/** Sets the internal y-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedY(const Kernel::cow_ptr<HistogramY> &y) & {
  if (yMode() == YMode::Uninitialized)
    throw std::logic_error("Histogram::setSharedY: YMode is not set and cannot be determined");
  if (y)
    checkSize(*y);
  m_y = y;
}

/** Sets the internal e-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedE(const Kernel::cow_ptr<HistogramE> &e) & {
  if (e)
    checkSize(*e);
  m_e = e;
}

/** Sets the internal dx-data pointer of the Histogram.

  Throws if the size does not match the current size. */
void Histogram::setSharedDx(const Kernel::cow_ptr<HistogramDx> &Dx) & {
  // Setting a NULL Dx is fine, this disables x errors.
  // Note that we compare with m_x -- m_dx might be NULL.
  PointStandardDeviations pointsSD(Dx);
  if (pointsSD)
    checkSize(pointsSD);
  m_dx = pointsSD.cowData();
}

/// Converts the histogram storage mode into YMode::Counts
void Histogram::convertToCounts() {
  if (yMode() == YMode::Counts)
    return;
  const auto &X = x();
  auto &Y = mutableY();
  auto &E = mutableE();
  for (size_t i = 0; i < Y.size(); ++i) {
    double width = X[i + 1] - X[i];
    Y[i] *= width;
    E[i] *= width;
  }
  m_yMode = YMode::Counts;
}

/// Converts the histogram storage mode into YMode::Frequencies
void Histogram::convertToFrequencies() {
  if (yMode() == YMode::Frequencies)
    return;
  const auto &X = x();
  auto &Y = mutableY();
  auto &E = mutableE();
  for (size_t i = 0; i < Y.size(); ++i) {
    double width = X[i + 1] - X[i];
    Y[i] /= width;
    E[i] /= width;
  }
  m_yMode = YMode::Frequencies;
}

template <> void Histogram::initX(const Points &x) {
  m_xMode = XMode::Points;
  m_x = x.cowData();
}

template <> void Histogram::initX(const BinEdges &x) {
  m_xMode = XMode::BinEdges;
  m_x = x.cowData();
  if (m_x->size() == 1)
    throw std::logic_error("Histogram: BinEdges size cannot be 1");
}

template <> void Histogram::setValues(const Counts &y) {
  m_yMode = YMode::Counts;
  setCounts(y);
}

template <> void Histogram::setValues(const Frequencies &y) {
  m_yMode = YMode::Frequencies;
  setFrequencies(y);
}

template <> void Histogram::setUncertainties(const CountVariances &e) { setCountVariances(e); }

template <> void Histogram::setUncertainties(const CountStandardDeviations &e) { setCountStandardDeviations(e); }

template <> void Histogram::setUncertainties(const FrequencyVariances &e) { setFrequencyVariances(e); }

template <> void Histogram::setUncertainties(const FrequencyStandardDeviations &e) {
  setFrequencyStandardDeviations(e);
}

void Histogram::checkAndSetYModeCounts() {
  if (yMode() == YMode::Frequencies)
    throw std::logic_error("Histogram: Y is storing Frequencies, modifying "
                           "Counts is not possible.");
  m_yMode = YMode::Counts;
}

void Histogram::checkAndSetYModeFrequencies() {
  if (yMode() == YMode::Counts)
    throw std::logic_error("Histogram: Y is storing Counts, modifying "
                           "Frequencies is not possible.");
  m_yMode = YMode::Frequencies;
}

template <> void Histogram::checkSize(const BinEdges &binEdges) const {
  size_t target = m_x->size();
  // 0 points -> 0 edges, otherwise edges are 1 more than points.
  if (xMode() == XMode::Points && target > 0)
    target++;
  if (target != binEdges.size()) {
    std::stringstream msg;
    msg << "Histogram: size mismatch of BinEdges: (" << target << " != " << binEdges.size() << ")";
    throw std::logic_error(msg.str());
  }
}

/** Resets the size of the internal x, dx, y, and e data structures

  The argument refers to the requested y length. All other lengths are
  then inferred automatically. The resize behaviour follows
  std::vector::resize which either truncates the current values
  or applies zero padding. */
void Histogram::resize(size_t n) {
  auto newXSize = (xMode() == XMode::Points || n == 0) ? n : n + 1;

  m_x.access().mutableRawData().resize(newXSize);
  if (m_y) {
    m_y.access().mutableRawData().resize(n);
  }

  if (m_e) {
    m_e.access().mutableRawData().resize(n);
  }

  if (m_dx) {
    m_dx.access().mutableRawData().resize(n);
  }
}

HistogramIterator Histogram::begin() const & { return HistogramIterator(*this, 0); }

HistogramIterator Histogram::end() const & { return HistogramIterator(*this, size()); }

} // namespace Mantid::HistogramData
