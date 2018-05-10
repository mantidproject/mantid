#ifndef MANTID_HISTOGRAMDATA_VALIDATION_H_
#define MANTID_HISTOGRAMDATA_VALIDATION_H_

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <stdexcept>

namespace Mantid {
namespace HistogramData {

class HistogramX;
class HistogramY;
class HistogramE;

namespace detail {

template <class TargetType> struct Validator {
  template <class T> static bool isValid(const T &) { return true; }
  template <class T> static void checkValidity(const T &) {}
};

template <> struct Validator<HistogramX> {
  template <class T> static bool isValid(const T &data);
  template <class T> static void checkValidity(const T &data);
};

template <> struct Validator<HistogramY> {
  template <class T> static bool isValid(const T &data);
  template <class T> static void checkValidity(const T &data);
};

template <> struct Validator<HistogramE> {
  template <class T> static bool isValid(const T &data);
  template <class T> static void checkValidity(const T &data);
};

template <class T> bool Validator<HistogramX>::isValid(const T &data) {
  auto it = std::find_if_not(data.begin(), data.end(),
                             [](const double d) { return std::isnan(d); });
  if (it == data.end())
    return true;
  for (; it < data.end() - 1; ++it) {
    if (std::isnan(*(it + 1)))
      break;
    double delta = *(it + 1) - *it;
    // Not 0.0, not denormal
    if (delta < DBL_MIN)
      return false;
  }
  ++it;
  // after first NAN everything must be NAN
  return std::find_if_not(it, data.end(), [](const double d) {
           return std::isnan(d);
         }) == data.end();
}

template <class T> void Validator<HistogramX>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramX");
}

template <class T> bool Validator<HistogramY>::isValid(const T &data) {
  auto result = std::find_if(data.begin(), data.end(),
                             [](const double y) { return std::isinf(y); });
  return result == data.end();
}

template <class T> void Validator<HistogramY>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramY");
}

template <class T> bool Validator<HistogramE>::isValid(const T &data) {
  auto result = std::find_if(data.begin(), data.end(), [](const double e) {
    return e < 0.0 || std::isinf(e);
  });
  return result == data.end();
}

template <class T> void Validator<HistogramE>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramE");
}
} // namespace detail
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_VALIDATION_H_ */
