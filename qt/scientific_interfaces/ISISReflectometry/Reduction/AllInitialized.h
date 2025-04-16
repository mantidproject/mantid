// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/optional.hpp>
#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename Param> bool allInitialized(std::optional<Param> const &param) { return param.has_value(); }

template <typename FirstParam, typename SecondParam, typename... Params>
bool allInitialized(std::optional<FirstParam> const &first, std::optional<SecondParam> const &second,
                    std::optional<Params> const &...params) {
  return first.has_value() && allInitialized(second, params...);
}

template <typename Result, typename... Params>
std::optional<Result> makeIfAllInitialized(std::optional<Params> const &...params) {
  if (allInitialized(params...)) {
    return Result(params.value()...);
  }
  return std::nullopt;
}

template <typename... Params> bool allInitializedPairs(Params... args) { return (... && args); }

template <typename Result, typename... Params>
std::optional<Result> makeIfAllInitializedPairs(std::optional<Params> const &...params) {
  return Result(params.value()...);
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
