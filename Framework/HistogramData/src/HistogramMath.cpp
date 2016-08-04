#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramMath.h"

namespace Mantid {
namespace HistogramData {

/** Scales data in histogram by constant factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 *uncertainties remain unchanged. */
Histogram &operator*=(Histogram &histogram, const double factor) {
  histogram.mutableY() *= factor;
  histogram.mutableE() *= factor;
  return histogram;
}

/** Divides data in histogram by constant factor.
 *
 * Uncertainties are divided by the same factor, such that the *relative*
 * uncertainties remain unchanged. */
Histogram &operator/=(Histogram &histogram, const double factor) {
  return histogram *= 1.0 / factor;
}

/** Scales data in histogram by constant factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 *uncertainties remain unchanged. */
Histogram operator*(Histogram histogram, const double factor) {
  return histogram *= factor;
}

/** Scales data in histogram by constant factor.
 *
 * Uncertainties are scaled by the same factor, such that the *relative*
 *uncertainties remain unchanged. */
Histogram operator*(const double factor, Histogram histogram) {
  return histogram *= factor;
}

/** Dividies data in histogram by constant factor.
 *
 * Uncertainties are divided by the same factor, such that the *relative*
 * uncertainties remain unchanged. */
Histogram operator/(Histogram histogram, const double factor) {
  return histogram *= 1.0 / factor;
}

} // namespace HistogramData
} // namespace Mantid
