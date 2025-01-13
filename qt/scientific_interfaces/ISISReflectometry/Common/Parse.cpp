// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Parse.h"
#include <cctype>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

bool isEntirelyWhitespace(std::string const &string) {
  return std::all_of(string.cbegin(), string.cend(), [](unsigned char c) { return std::isspace(c); });
}

std::optional<double> parseDouble(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stod(string, &end);
    if (end == string.size())
      return result;
    else
      return std::nullopt;
  } catch (std::invalid_argument &) {
    return std::nullopt;
  } catch (std::out_of_range &) {
    return std::nullopt;
  }
}

std::optional<double> parseNonNegativeDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.has_value() && maybeNegative.value() >= 0.0)
    return maybeNegative.value();
  else
    return std::nullopt;
}

std::optional<double> parseNonNegativeNonZeroDouble(std::string string) {
  auto maybeNegative = parseDouble(std::move(string));
  if (maybeNegative.has_value() && maybeNegative.value() > 0.0)
    return maybeNegative.value();
  else
    return std::nullopt;
}

std::optional<int> parseInt(std::string string) {
  boost::trim(string);
  auto end = std::size_t();
  try {
    auto result = std::stoi(string, &end);
    if (end == string.size())
      return result;
    else
      return std::nullopt;
  } catch (std::invalid_argument &) {
    return std::nullopt;
  } catch (std::out_of_range &) {
    return std::nullopt;
  }
}

std::optional<int> parseNonNegativeInt(std::string string) {
  auto maybeNegative = parseInt(std::move(string));
  if (maybeNegative.has_value() && maybeNegative.value() >= 0)
    return maybeNegative.value();
  else
    return std::nullopt;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
