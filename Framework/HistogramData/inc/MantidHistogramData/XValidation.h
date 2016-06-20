#ifndef MANTID_HISTOGRAMDATA_XVALIDATION_H_
#define MANTID_HISTOGRAMDATA_XVALIDATION_H_

#include "MantidHistogramData/Validation.h"

#include <cmath>
#include <numeric>

namespace Mantid {
namespace HistogramData {

class BinEdges;
class HistogramX;
class Points;

namespace detail {
template <class T> bool Validator<HistogramX>::isValid(const T &data) {
  if (data.size() == 1)
    return std::isfinite(data[0]);
  for (size_t i = 1; i < data.size(); ++i) {
    double delta = data[i] - data[i - 1];
    // Not 0.0, not denormal, not inf, not nan.
    if (delta < 0.0 || !std::isnormal(delta))
      return false;
  }
  return true;
}
template <class T> void Validator<HistogramX>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramX");
}
}

template <class T, typename std::enable_if<
                       std::is_same<HistogramX, T>::value ||
                       std::is_same<BinEdges, T>::value ||
                       std::is_same<Points, T>::value>::type * = nullptr>
bool isValid(const T &xData) {
  return detail::Validator<HistogramX>::isValid(xData);
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_XVALIDATION_H_ */
