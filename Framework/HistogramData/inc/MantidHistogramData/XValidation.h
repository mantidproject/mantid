// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_HISTOGRAMDATA_XVALIDATION_H_
#define MANTID_HISTOGRAMDATA_XVALIDATION_H_

#include "MantidHistogramData/Validation.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class Points;

template <class T, typename std::enable_if<std::is_same<HistogramX, T>::value ||
                                           std::is_same<BinEdges, T>::value ||
                                           std::is_same<Points, T>::value>::type
                       * = nullptr>
bool isValid(const T &xData) {
  return detail::Validator<HistogramX>::isValid(xData);
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_XVALIDATION_H_ */
