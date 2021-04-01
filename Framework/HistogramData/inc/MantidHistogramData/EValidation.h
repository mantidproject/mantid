// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/Validation.h"

namespace Mantid {
namespace HistogramData {

class CountStandardDeviations;
class CountVariances;
class FrequencyStandardDeviations;
class FrequencyVariances;

template <class T, typename std::enable_if<
                       std::is_same<HistogramE, T>::value || std::is_same<CountStandardDeviations, T>::value ||
                       std::is_same<CountVariances, T>::value || std::is_same<FrequencyStandardDeviations, T>::value ||
                       std::is_same<FrequencyVariances, T>::value>::type * = nullptr>
bool isValid(const T &eData) {
  return detail::Validator<HistogramE>::isValid(eData);
}

} // namespace HistogramData
} // namespace Mantid
