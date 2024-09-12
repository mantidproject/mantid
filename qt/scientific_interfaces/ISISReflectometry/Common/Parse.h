// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllConfig.h"
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double> parseDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double> parseNonNegativeDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double> parseNonNegativeNonZeroDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int> parseInt(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int> parseNonNegativeInt(std::string string);

bool MANTIDQT_ISISREFLECTOMETRY_DLL isEntirelyWhitespace(std::string const &string);

template <typename ParseItemFunction>
boost::optional<std::vector<typename std::invoke_result<ParseItemFunction, std::string const &>::type::value_type>>
parseList(std::string commaSeparatedValues, ParseItemFunction parseItem) {
  using ParsedItem = typename std::invoke_result<ParseItemFunction, std::string const &>::type::value_type;
  if (!commaSeparatedValues.empty()) {
    auto items = std::vector<std::string>();
    boost::algorithm::split(items, commaSeparatedValues, boost::is_any_of(","));
    auto parsedItems = std::vector<ParsedItem>();
    parsedItems.reserve(items.size());
    for (auto const &item : items) {
      auto maybeParsedItem = parseItem(item);
      if (maybeParsedItem.is_initialized())
        parsedItems.emplace_back(maybeParsedItem.get());
      else
        return boost::none;
    }
    return parsedItems;
  } else {
    return std::vector<ParsedItem>();
  }
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
