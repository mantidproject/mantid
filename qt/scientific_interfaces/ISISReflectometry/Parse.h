/**
Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#ifndef MANTID_CUSTOMINTERFACES_PARSE_H_
#define MANTID_CUSTOMINTERFACES_PARSE_H_
#include "DllConfig.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

namespace MantidQt {
namespace CustomInterfaces {

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseNonNegativeDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<double>
parseNonNegativeNonZeroDouble(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int>
parseInt(std::string string);

MANTIDQT_ISISREFLECTOMETRY_DLL boost::optional<int>
parseNonNegativeInt(std::string string);

bool MANTIDQT_ISISREFLECTOMETRY_DLL
isEntirelyWhitespace(std::string const &string);

template <typename ParseItemFunction>
boost::optional<std::vector<typename std::result_of<
    ParseItemFunction(std::string const &)>::type::value_type>>
parseList(std::string commaSeparatedValues, ParseItemFunction parseItem) {
  using ParsedItem = typename std::result_of<ParseItemFunction(
      std::string const &)>::type::value_type;
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
}
}
#endif // MANTID_CUSTOMINTERFACES_PARSE_H_
