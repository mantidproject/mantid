// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#ifndef MANTIDQTMANTIDWIDGETS_ASSERTORTHROW_H_
#define MANTIDQTMANTIDWIDGETS_ASSERTORTHROW_H_
#include <stdexcept>
#include <string>

inline void assertOrThrow(bool condition, std::string const &message) {
  if (!condition)
    throw std::runtime_error(message);
}

#endif
