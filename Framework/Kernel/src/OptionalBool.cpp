#include "MantidKernel/OptionalBool.h"

#include <ostream>
#include <utility>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
OptionalBool::OptionalBool() : m_arg(Unset) {}

OptionalBool::OptionalBool(bool arg) { m_arg = arg ? True : False; }

OptionalBool::OptionalBool(Value arg) : m_arg(arg) {}

bool OptionalBool::operator==(const OptionalBool &other) const {
  return m_arg == other.getValue();
}

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

const std::string OptionalBool::StrUnset = "Unset";
const std::string OptionalBool::StrFalse = "False";
const std::string OptionalBool::StrTrue = "True";

std::map<std::string, OptionalBool::Value> OptionalBool::strToEmumMap() {
  return {{StrUnset, OptionalBool::Unset},
          {StrFalse, OptionalBool::False},
          {StrTrue, OptionalBool::True}};
}

std::map<OptionalBool::Value, std::string> OptionalBool::enumToStrMap() {
  std::map<Value, std::string> map;
  auto opposite = strToEmumMap();
  for (auto &oppositePair : opposite) {
    map.emplace(oppositePair.second, oppositePair.first);
  }
  return map;
}

} // namespace Kernel
} // namespace Mantid
