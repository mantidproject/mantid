// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Parse.h"
#include <cctype>

namespace MantidQt {
namespace CustomInterfaces {

bool isEntirelyWhitespace(std::string const &string) {
  return std::all_of(string.cbegin(), string.cend(),
                     [](unsigned char c) { return std::isspace(c); });
}

boost::optional<double> parseDouble(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stod(string, &end);
    if (end == string.size())
      return result;
    else
      return boost::none;
  } catch (std::invalid_argument &) {
    return boost::none;
  } catch (std::out_of_range &) {
    return boost::none;
  }
}

boost::optional<double> parseNonNegativeDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() >= 0.0)
    return maybeNegative.get();
  else
    return boost::none;
}

boost::optional<double> parseNonNegativeNonZeroDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() > 0.0)
    return maybeNegative.get();
  else
    return boost::none;
}

boost::optional<int> parseInt(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stoi(string, &end);
    if (end == string.size())
      return result;
    else
      return boost::none;
  } catch (std::invalid_argument &) {
    return boost::none;
  } catch (std::out_of_range &) {
    return boost::none;
  }
}

boost::optional<int> parseNonNegativeInt(std::string string) {
  auto maybeNegative = parseInt(std::move(string));
  if (maybeNegative.is_initialized() && maybeNegative.get() >= 0)
    return maybeNegative.get();
  else
    return boost::none;
}
} // namespace CustomInterfaces
} // namespace MantidQt
