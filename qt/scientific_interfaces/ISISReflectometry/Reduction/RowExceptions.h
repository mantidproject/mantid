// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <exception>
#include <string>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

struct RowNotFoundException : public std::out_of_range {
public:
  RowNotFoundException(std::string s) : std::out_of_range(std::move(s)) {};
};

struct MultipleRowsFoundException : public std::length_error {
public:
  MultipleRowsFoundException(std::string s) : std::length_error(std::move(s)) {};
};

struct InvalidTableException : public std::runtime_error {
public:
  InvalidTableException(std::string s) : std::runtime_error(std::move(s)) {};
};

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
