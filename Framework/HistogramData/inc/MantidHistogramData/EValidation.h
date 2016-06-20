#ifndef MANTID_HISTOGRAMDATA_EVALIDATION_H_
#define MANTID_HISTOGRAMDATA_EVALIDATION_H_

#include "MantidHistogramData/Validation.h"

namespace Mantid {
namespace HistogramData {

class BinEdgeStandardDeviations;
class BinEdgeVariances;
class PointStandardDeviations;
class PointVariances;

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
