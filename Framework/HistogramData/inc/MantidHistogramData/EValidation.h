#ifndef MANTID_HISTOGRAMDATA_EVALIDATION_H_
#define MANTID_HISTOGRAMDATA_EVALIDATION_H_

#include "MantidHistogramData/Validation.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace Mantid {
namespace HistogramData {

class HistogramE;
class BinEdgeStandardDeviations;
class BinEdgeVariances;
class PointStandardDeviations;
class PointVariances;

namespace detail {
template <class T> bool Validator<HistogramE>::isValid(const T &data) {
  auto result = std::find_if(data.begin(), data.end(), [](const double &e) {
    return e < 0.0 || !std::isfinite(e);
  });
  return result == data.end();
}
template <class T> void Validator<HistogramE>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramE");
}
}

template <class T,
          typename std::enable_if<
              std::is_same<HistogramE, T>::value ||
              std::is_same<BinEdgeStandardDeviations, T>::value ||
              std::is_same<BinEdgeVariances, T>::value ||
              std::is_same<PointStandardDeviations, T>::value ||
              std::is_same<PointVariances, T>::value>::type * = nullptr>
bool isValid(const T &eData) {
  return detail::Validator<HistogramE>::isValid(eData);
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_EVALIDATION_H_ */
