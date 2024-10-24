// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

template <typename Param> bool allInitialized(boost::optional<Param> const &param) { return param.is_initialized(); }

template <typename FirstParam, typename SecondParam, typename... Params>
bool allInitialized(boost::optional<FirstParam> const &first, boost::optional<SecondParam> const &second,
                    boost::optional<Params> const &...params) {
  return first.is_initialized() && allInitialized(second, params...);
}

template <typename Result, typename... Params>
boost::optional<Result> makeIfAllInitialized(boost::optional<Params> const &...params) {
  if (allInitialized(params...))
    return Result(params.get()...);
  else
    return boost::none;
}

} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
