// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/OptionalBool.h"

#include <json/value.h>

#include <ostream>
#include <utility>

namespace Mantid {
namespace Kernel {

const std::string OptionalBool::StrUnset = "Unset";
const std::string OptionalBool::StrFalse = "False";
const std::string OptionalBool::StrTrue = "True";

OptionalBool::OptionalBool() : m_arg(Unset) {}

OptionalBool::OptionalBool(bool arg) { m_arg = arg ? True : False; }

OptionalBool::OptionalBool(Value arg) : m_arg(arg) {}

bool OptionalBool::operator==(const OptionalBool &other) const { return m_arg == other.getValue(); }

OptionalBool::Value OptionalBool::getValue() const { return m_arg; }

std::ostream &operator<<(std::ostream &os, OptionalBool const &object) {
  os << OptionalBool::enumToStrMap()[object.getValue()];
  return os;
}

std::istream &operator>>(std::istream &istream, OptionalBool &object) {
  std::string result;
  istream >> result;
  object.m_arg = OptionalBool::strToEmumMap()[result];
  return istream;
}

std::map<std::string, OptionalBool::Value> OptionalBool::strToEmumMap() {
  return {{StrUnset, OptionalBool::Unset}, {StrFalse, OptionalBool::False}, {StrTrue, OptionalBool::True}};
}

std::map<OptionalBool::Value, std::string> OptionalBool::enumToStrMap() {
  std::map<Value, std::string> map;
  auto opposite = strToEmumMap();
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

} // namespace Kernel
} // namespace Mantid
