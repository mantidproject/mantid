// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "DllOption.h"
#include <string>

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON Hint {
public:
  Hint(std::string word, std::string description);
  std::string const &word() const;
  std::string const &description() const;

private:
  std::string m_word;
  std::string m_description;
};

EXPORT_OPT_MANTIDQT_COMMON bool operator==(Hint const &lhs, Hint const &rhs);
EXPORT_OPT_MANTIDQT_COMMON bool operator!=(Hint const &lhs, Hint const &rhs);
} // namespace MantidWidgets
} // namespace MantidQt
