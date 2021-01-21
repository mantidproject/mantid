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

class Counts;
class Frequencies;

template <class T, typename std::enable_if<std::is_same<HistogramY, T>::value || std::is_same<Counts, T>::value ||
                                           std::is_same<Frequencies, T>::value>::type * = nullptr>
bool isValid(const T &eData) {
  return detail::Validator<HistogramY>::isValid(eData);
}

} // namespace HistogramData
} // namespace Mantid
