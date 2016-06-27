#ifndef MANTID_HISTOGRAMDATA_VALIDATION_H_
#define MANTID_HISTOGRAMDATA_VALIDATION_H_

#include <algorithm>
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

template <class T> bool Validator<HistogramY>::isValid(const T &data) {
  auto result = std::find_if(data.begin(), data.end(),
                             [](const double &y) { return std::isinf(y); });
  return result == data.end();
}

template <class T> void Validator<HistogramY>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramY");
}

template <class T> bool Validator<HistogramE>::isValid(const T &data) {
  auto result = std::find_if(data.begin(), data.end(), [](const double &e) {
    return e < 0.0 || std::isinf(e);
  });
  return result == data.end();
}

template <class T> void Validator<HistogramE>::checkValidity(const T &data) {
  if (!isValid(data))
    throw std::runtime_error(
        "Invalid data found during construction of HistogramE");
}
}
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_VALIDATION_H_ */
