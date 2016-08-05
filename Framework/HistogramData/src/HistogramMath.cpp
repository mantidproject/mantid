#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramMath.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Mantid {
namespace HistogramData {

/** Scales data in histogram by constant (non-negative) factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 * uncertainties remain unchanged. */
Histogram &operator*=(Histogram &histogram, const double factor) {
  if (factor < 0.0 || !std::isfinite(factor))
    throw std::runtime_error("Invalid operation: Cannot scale Histogram by "
                             "negative or infinite factor");
  histogram.mutableY() *= factor;
  histogram.mutableE() *= factor;
  return histogram;
}

/** Divides data in histogram by constant (positive) factor.
 *
 * Uncertainties are divided by the same factor, such that the *relative*
 * uncertainties remain unchanged. */
Histogram &operator/=(Histogram &histogram, const double factor) {
  return histogram *= 1.0 / factor;
}

/** Scales data in histogram by constant (non-negative) factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 *uncertainties remain unchanged. */
Histogram operator*(Histogram histogram, const double factor) {
  return histogram *= factor;
}

/** Scales data in histogram by constant (non-negative) factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 *uncertainties remain unchanged. */
Histogram operator*(const double factor, Histogram histogram) {
  return histogram *= factor;
}

/** Dividies data in histogram by constant (positive) factor.
 *
 * Uncertainties are divided by the same factor, such that the *relative*
 * uncertainties remain unchanged. */
Histogram operator/(Histogram histogram, const double factor) {
  return histogram *= 1.0 / factor;
}

namespace {
void checkSameXYMode(const Histogram &hist1, const Histogram &hist2) {
  if (hist1.xMode() != hist2.xMode())
    throw std::runtime_error("Invalid operation: Histogram::XModes must match");
  if (hist1.yMode() != hist2.yMode())
    throw std::runtime_error("Invalid operation: Histogram::YModes must match");
}

void checkSameX(const Histogram &hist1, const Histogram &hist2) {
  if (!(hist1.sharedX() == hist2.sharedX()) &&
      (hist1.x().rawData() != hist2.x().rawData()))
    throw std::runtime_error("Invalid operation: Histogram X data must match");
}
}

/// Adds data in other Histogram to this Histogram, propagating uncertainties.
Histogram &operator+=(Histogram &histogram, const Histogram &other) {
  checkSameXYMode(histogram, other);
  checkSameX(histogram, other);
  histogram.mutableY() += other.y();
  std::transform(histogram.e().cbegin(), histogram.e().cend(),
                 other.e().begin(), histogram.mutableE().begin(),
                 [](const double &lhs, const double &rhs)
                     -> double { return std::sqrt(lhs * lhs + rhs * rhs); });
  return histogram;
}

/// Subtracts data in other Histogram from this Histogram, propagating
/// uncertainties.
Histogram &operator-=(Histogram &histogram, const Histogram &other) {
  checkSameXYMode(histogram, other);
  checkSameX(histogram, other);
  histogram.mutableY() -= other.y();
  std::transform(histogram.e().cbegin(), histogram.e().cend(),
                 other.e().begin(), histogram.mutableE().begin(),
                 [](const double &lhs, const double &rhs)
                     -> double { return std::sqrt(lhs * lhs + rhs * rhs); });
  return histogram;
}

/// Adds data from two Histograms, propagating uncertainties.
Histogram operator+(Histogram histogram, const Histogram &other) {
  return histogram += other;
}

/// Subtracts data from two Histograms, propagating uncertainties.
Histogram operator-(Histogram histogram, const Histogram &other) {
  return histogram -= other;
}

} // namespace HistogramData
} // namespace Mantid
