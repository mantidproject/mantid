#ifndef MANTID_HISTOGRAMDATA_EVALIDATION_H_
#define MANTID_HISTOGRAMDATA_EVALIDATION_H_

#include "MantidHistogramData/Validation.h"

namespace Mantid {
namespace HistogramData {

class CountStandardDeviations;
class CountVariances;
class FrequencyStandardDeviations;
class FrequencyVariances;

template <class T,
          typename std::enable_if<
              std::is_same<HistogramE, T>::value ||
              std::is_same<CountStandardDeviations, T>::value ||
              std::is_same<CountVariances, T>::value ||
              std::is_same<FrequencyStandardDeviations, T>::value ||
              std::is_same<FrequencyVariances, T>::value>::type * = nullptr>
bool isValid(const T &eData) {
  return detail::Validator<HistogramE>::isValid(eData);
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_EVALIDATION_H_ */
