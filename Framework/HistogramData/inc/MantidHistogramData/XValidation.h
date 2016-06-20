#ifndef MANTID_HISTOGRAMDATA_XVALIDATION_H_
#define MANTID_HISTOGRAMDATA_XVALIDATION_H_

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"

#include <cmath>
#include <numeric>

namespace Mantid {
namespace HistogramData {

template <class T, typename std::enable_if<
                       std::is_same<HistogramX, T>::value ||
                       std::is_same<BinEdges, T>::value ||
                       std::is_same<Points, T>::value>::type * = nullptr>
bool isValid(const T &xData) {
  if (xData.size() == 1)
    return std::isfinite(xData[0]);
  for (size_t i = 1; i < xData.size(); ++i) {
    double delta = xData[i] - xData[i - 1];
    // Not 0.0, not denormal, not inf, not nan.
    if (delta < 0.0 || !std::isnormal(delta))
      return false;
  }
  return true;
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_XVALIDATION_H_ */
