#ifndef MANTID_HISTOGRAMDATA_VALIDATION_H_
#define MANTID_HISTOGRAMDATA_VALIDATION_H_

namespace Mantid {
namespace HistogramData {
class HistogramX;
namespace detail {

template <class TargetType> struct Validator {
  template <class T> static bool isValid(const T &) { return true; }
  template <class T> static void checkValidity(const T &) {}
};

// Defined in XValidation.h
template <> struct Validator<HistogramX> {
  template <class T> static bool isValid(const T &data);
  template <class T> static void checkValidity(const T &data);
};
}
} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_VALIDATION_H_ */
