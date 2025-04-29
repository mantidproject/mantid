// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/OptionalBool.h"

#include <json/value.h>

#include <bits/stdc++.h>
#include <ostream>
#include <utility>

namespace Mantid::Kernel {

const std::string OptionalBool::StrFalse = "False";
const std::string OptionalBool::StrTrue = "True";
const std::string OptionalBool::StrUnset = "Unset";

OptionalBool::Value OptionalBool::Validate(const std::string &arg) {
  auto l_toLower = [](char c) -> char { return (c >= 'A' && c <= 'Z' ? c + 0x20 : c); };
  std::string argLower = arg;
  std::transform(arg.begin(), arg.end(), argLower.begin(), l_toLower);
  if (argLower == "false") {
    return False;
  } else if (argLower == "true") {
    return True;
  } else if (argLower == "unset") {
    return Unset;
  } else {
    throw std::invalid_argument("Invalid value for OptionalBool: " + arg);
  }
}

OptionalBool::OptionalBool() : m_arg(Unset) {}
OptionalBool::OptionalBool(bool arg) : m_arg(OptionalBool::Value(arg)) {}
OptionalBool::OptionalBool(OptionalBool::Value arg) : m_arg(arg) {}
OptionalBool::OptionalBool(std::string arg) : m_arg(Validate(arg)) {}
OptionalBool::OptionalBool(char const *arg) : m_arg(Validate(std::string(arg))) {}
OptionalBool::OptionalBool(const int arg)
    : m_arg(arg == 0   ? OptionalBool::False
            : arg == 1 ? OptionalBool::True
                       : throw std::invalid_argument("Invalid value for OptionalBool: " + std::to_string(arg) +
                                                     "\nAccepted values are 0 or 1")) {}
OptionalBool &OptionalBool::operator=(std::string const &arg) {
  m_arg = OptionalBool::Validate(arg);
  return *this;
}
OptionalBool &OptionalBool::operator=(char const *arg) {
  m_arg = OptionalBool::Validate(std::string(arg));
  return *this;
}
OptionalBool &OptionalBool::operator=(const int arg) {
  m_arg = arg == 0   ? OptionalBool::False
          : arg == 1 ? OptionalBool::True
                     : throw std::invalid_argument("Invalid value for OptionalBool: " + std::to_string(arg) +
                                                   "\nAccepted values are 0 or 1");
  return *this;
}

bool OptionalBool::operator==(const OptionalBool &other) const { return m_arg == other.getValue(); }

bool OptionalBool::operator!=(const OptionalBool &other) const { return m_arg != other.getValue(); }

OptionalBool::Value OptionalBool::getValue() const { return m_arg; }

std::ostream &operator<<(std::ostream &os, OptionalBool const &object) {
  os << OptionalBool::enumToStrMap()[object.getValue()];
  return os;
}

std::istream &operator>>(std::istream &istream, OptionalBool &object) {
  std::string result;
  istream >> result;
  object.m_arg = OptionalBool::strToEnumMap()[result];
  return istream;
}

std::map<std::string, OptionalBool::Value> OptionalBool::strToEnumMap() {
  return {{StrUnset, OptionalBool::Unset}, {StrFalse, OptionalBool::False}, {StrTrue, OptionalBool::True}};
}

std::map<OptionalBool::Value, std::string> OptionalBool::enumToStrMap() {
  std::map<OptionalBool::Value, std::string> map;
  auto opposite = strToEnumMap();
  for (auto &oppositePair : opposite) {
    map.emplace(oppositePair.second, oppositePair.first);
  }
  return map;
}

/**
 * Encode an OptionalBool as a Json::Value. Throws as it's not clear how to
 * serialize this type.
 * @return A new Json::Value
 */
Json::Value encodeAsJson(const OptionalBool &value) {
  const auto enumValue = value.getValue();
  if (enumValue == OptionalBool::True)
    return Json::Value(true);
  else if (enumValue == OptionalBool::False)
    return Json::Value(false);
  else
    return Json::Value();
}

// Specialization of ToCpp for OptionalBool
namespace pwvjdetail {

bool ToCpp<OptionalBool>::operator()(const Json::Value &value) { return value.asBool(); }

} // namespace pwvjdetail

} // namespace Mantid::Kernel
