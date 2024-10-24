// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <algorithm>
#include <boost/optional.hpp>
#include <iterator>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename Container, typename Predicate>
boost::optional<int> indexOf(Container const &container, Predicate pred) {
  auto maybeItemIt = std::find_if(container.cbegin(), container.cend(), pred);
  if (maybeItemIt != container.cend())
    return static_cast<int>(std::distance(container.cbegin(), maybeItemIt));
  else
    return boost::none;
}

template <typename Container, typename ValueType>
boost::optional<int> indexOfValue(Container const &container, ValueType value) {
  auto maybeItemIt = std::find(container.cbegin(), container.cend(), value);
  if (maybeItemIt != container.cend())
    return static_cast<int>(std::distance(container.cbegin(), maybeItemIt));
  else
    return boost::none;
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
