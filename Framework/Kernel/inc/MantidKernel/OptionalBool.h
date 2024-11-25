// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/PropertyWithValueJSON.h"

#include <iosfwd>
#include <map>
#include <string>

namespace Json {
class Value;
}

namespace Mantid {
namespace Kernel {

/** OptionalBool : Tri-state bool. Defaults to unset.
 */
class MANTID_KERNEL_DLL OptionalBool {
public:
  enum Value { Unset, True, False };
  static std::map<std::string, Value> strToEmumMap();
  static std::map<Value, std::string> enumToStrMap();
  const static std::string StrUnset;
  const static std::string StrFalse;
  const static std::string StrTrue;

public:
  OptionalBool();
  OptionalBool(bool arg);
  OptionalBool(Value arg);
  virtual ~OptionalBool() = default;
  bool operator==(const OptionalBool &other) const;
  bool operator!=(const OptionalBool &other) const;
  Value getValue() const;

private:
  friend MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &os, OptionalBool const &object);
  friend MANTID_KERNEL_DLL std::istream &operator>>(std::istream &istream, OptionalBool &object);

  Value m_arg;
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &os, OptionalBool const &object);

MANTID_KERNEL_DLL std::istream &operator>>(std::istream &istream, OptionalBool &object);

/// Encode an OptionalBool as a Json::Value.
MANTID_KERNEL_DLL ::Json::Value encodeAsJson(const OptionalBool &);

namespace pwvjdetail {

template <> struct ToCpp<OptionalBool> {
  bool operator()(const Json::Value &value);
};

} // namespace pwvjdetail

} // namespace Kernel
} // namespace Mantid
