#include "MantidHistogramData/HistogramMath.h"
#include "MantidHistogramData/Histogram.h"

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
void checkSameXMode(const Histogram &hist1, const Histogram &hist2) {
  if (hist1.xMode() != hist2.xMode())
    throw std::runtime_error("Invalid operation: Histogram::XModes must match");
}

void checkSameYMode(const Histogram &hist1, const Histogram &hist2) {
  if (hist1.yMode() != hist2.yMode())
    throw std::runtime_error("Invalid operation: Histogram::YModes must match");
}

void checkSameX(const Histogram &hist1, const Histogram &hist2) {
  if (!(hist1.sharedX() == hist2.sharedX()) &&
      (hist1.x().rawData() != hist2.x().rawData()))
    throw std::runtime_error("Invalid operation: Histogram X data must match");
}
} // namespace

/// Adds data in other Histogram to this Histogram, propagating uncertainties.
Histogram &operator+=(Histogram &histogram, const Histogram &other) {
  checkSameXMode(histogram, other);
  checkSameYMode(histogram, other);
  checkSameX(histogram, other);
  histogram.mutableY() += other.y();
  std::transform(histogram.e().cbegin(), histogram.e().cend(),
                 other.e().begin(), histogram.mutableE().begin(),
                 [](const double &lhs, const double &rhs) -> double {
                   return std::sqrt(lhs * lhs + rhs * rhs);
                 });
  return histogram;
}

/// Subtracts data in other Histogram from this Histogram, propagating
/// uncertainties.
Histogram &operator-=(Histogram &histogram, const Histogram &other) {
  checkSameXMode(histogram, other);
  checkSameYMode(histogram, other);
  checkSameX(histogram, other);
  histogram.mutableY() -= other.y();
  std::transform(histogram.e().cbegin(), histogram.e().cend(),
                 other.e().begin(), histogram.mutableE().begin(),
                 [](const double &lhs, const double &rhs) -> double {
                   return std::sqrt(lhs * lhs + rhs * rhs);
                 });
  return histogram;
}

/// Multiplies data in other Histogram with this Histogram, propagating
/// uncertainties.
Histogram &operator*=(Histogram &histogram, const Histogram &other) {
  if (histogram.yMode() == Histogram::YMode::Counts &&
      other.yMode() == Histogram::YMode::Counts)
    throw std::runtime_error("Invalid operation: Cannot multiply two "
                             "histograms with YMode::Counts. Convert (one of "
                             "them) to a distribution (YMode::Frequencies).");
  checkSameXMode(histogram, other);
  checkSameX(histogram, other);

  auto &y1 = histogram.mutableY();
  auto &e1 = histogram.mutableE();
  const auto &y2 = other.y();
  const auto &e2 = other.e();
  for (size_t i = 0; i < y1.size(); ++i)
    e1[i] = sqrt(pow(e1[i] * y2[i], 2) + pow(e2[i] * y1[i], 2));
  y1 *= y2;
  if (other.yMode() == Histogram::YMode::Counts)
    histogram.setYMode(Histogram::YMode::Counts);
  return histogram;
}

/// Divides data in this Histogram by other Histogram, propagating
/// uncertainties.
Histogram &operator/=(Histogram &histogram, const Histogram &other) {
  if (histogram.yMode() == Histogram::YMode::Frequencies &&
      other.yMode() == Histogram::YMode::Counts)
    throw std::runtime_error(
        "Invalid operation: Cannot divide histogram with YMode::Frequencies by "
        "histogram with YMode::Counts. Convert numerator to YMode::Counts or "
        "denominator to a distribution (YMode::Frequencies).");
  checkSameXMode(histogram, other);
  checkSameX(histogram, other);

  auto &y1 = histogram.mutableY();
  auto &e1 = histogram.mutableE();
  const auto &y2 = other.y();
  const auto &e2 = other.e();
  for (size_t i = 0; i < y1.size(); ++i) {
    auto inv_y2 = 1.0 / y2[i];
    e1[i] = sqrt(pow(e1[i], 2) + pow(e2[i] * y1[i] * inv_y2, 2)) * fabs(inv_y2);
    y1[i] *= inv_y2;
  }
  if (histogram.yMode() == other.yMode())
    histogram.setYMode(Histogram::YMode::Frequencies);
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

/// Multiplies data from two Histograms, propagating uncertainties.
Histogram operator*(Histogram histogram, const Histogram &other) {
  return histogram *= other;
}

/// Divides data from two Histograms, propagating uncertainties.
Histogram operator/(Histogram histogram, const Histogram &other) {
  return histogram /= other;
}

} // namespace HistogramData
} // namespace Mantid
