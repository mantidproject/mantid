// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/variant.hpp>
#include <optional>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename T> std::optional<T> first(std::vector<T> const &values) {
  if (values.size() > 0) {
    return std::optional<T>(values[0]);
  }
  return std::nullopt;
}

/**
 * Operates on a variant<vector<Ts>...> extracting the first element and
 * returning it as a optional<variant<T>> where the optional is empty if
 * the vector held by the variant<vector<T>> held no values.
 */
template <typename... Ts> class FirstVisitor : public boost::static_visitor<std::optional<boost::variant<Ts...>>> {
public:
  template <typename T> std::optional<boost::variant<Ts...>> operator()(std::vector<T> const &values) const {
    auto value = first(values);
    if (value) {
      return boost::variant<Ts...>(value.get());
    }
    return std::nullopt;
  }
};
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
