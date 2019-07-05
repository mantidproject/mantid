// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ISISREFLECTOMETRY_VALUEOR_H
#define MANTID_ISISREFLECTOMETRY_VALUEOR_H
#include <boost/optional.hpp>

/**
 * This method is required to support RHEL7 since it uses boost 1.53.
 * which only has get_value_or unlike std::optional (c++17) or later boost
 * versions.
 *
 * Once RHEL7 support is dropped usages should be replaced with the more
 * readable .value_or member function.
 */
template <typename T, typename U>
T value_or(boost::optional<T> const &value, U &&ifEmpty) {
  return value.get_value_or(ifEmpty);
}

#endif // MANTID_ISISREFLECTOMETRY_VALUEOR_H
