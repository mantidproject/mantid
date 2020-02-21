// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_MAP_H
#define MANTID_ISISREFLECTOMETRY_MAP_H
#include <algorithm>
#include <boost/optional.hpp>
#include <iterator>
#include <sstream>
#include <type_traits>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename Container, typename Transform,
          typename Out = typename std::result_of<
              Transform(typename Container::value_type)>::type>
std::vector<Out> map(Container const &in, Transform transform) {
  auto out = std::vector<Out>();
  out.reserve(in.size());
  std::transform(in.cbegin(), in.cend(), std::back_inserter(out), transform);
  return out;
}

template <typename In, typename Transform,
          typename Out = typename std::result_of<Transform(In)>::type>
boost::optional<Out> map(boost::optional<In> const &in, Transform transform) {
  if (in.is_initialized())
    return transform(in.get());
  else
    return boost::none;
}

/** Converts an optional value to string
 *
 * @param maybeValue optional value
 * @return The value as a string or an empty string
 *
 */
template <typename T>
std::string optionalToString(boost::optional<T> maybeValue) {
  return map(maybeValue,
             [](T const &value) -> std::string {
               return std::to_string(value);
             })
      .get_value_or(std::string());
}

/** Converts value to string with specified precision
 *
 * @param value input value
 * @param precision desired precision
 * @return The value as a string with specified precision
 *
 */
template <typename T> std::string valueToString(T value, int precision) {
  std::ostringstream result;
  result.precision(precision);
  result << std::fixed << value;
  return result.str();
}

/** Converts value to string with optional precision
 *
 * @param value input value
 * @param precision optional precision
 * @return The value as a string (with specified precision if given)
 *
 */
template <typename T>
std::string valueToString(T value, boost::optional<int> precision) {
  if (precision.is_initialized())
    return valueToString(value, precision.get());
  return std::to_string(value);
}

/** Converts optional value to string with optional precision
 *
 * @param maybeValue optional input value
 * @param precision optional output precision
 * @return The value as a string (with specified precision if given) or empty
 * string
 *
 */
template <typename T>
std::string optionalToString(boost::optional<T> maybeValue,
                             boost::optional<int> precision) {
  if (maybeValue.is_initialized()) {
    if (precision.is_initialized()) {
      return valueToString(maybeValue.get(), precision.get());
    }
    return optionalToString(maybeValue);
  }
  return std::string();
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_ISISREFLECTOMETRY_MAP_H
