// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/optional.hpp>
#include <optional>
#include <ostream>

// Here we provide a printer for std::optional that can be used
// in cases where the output is not interesting, e.g.
// it is a requirement by GoogleMock

namespace boost {

/// Simple prints the address of the optional object
template <class CharType, class CharTrait, class OptionalValueType>
inline std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &os,
                                                           std::optional<OptionalValueType> const &value) {
  return (os << "std::optional @ " << reinterpret_cast<const void *>(&value));
}

/// Simple prints the address of the optional object - this can be removed once boost::optional is converted to
/// std::optional
template <class CharType, class CharTrait, class OptionalValueType>
inline std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &os,
                                                           boost::optional<OptionalValueType> const &value) {
  return (os << "boost::optional @ " << reinterpret_cast<const void *>(&value));
}
} // namespace boost
