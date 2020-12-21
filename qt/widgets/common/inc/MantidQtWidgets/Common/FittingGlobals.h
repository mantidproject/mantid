// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <string>

namespace MantidQt {
namespace MantidWidgets {

struct GlobalTie {

  GlobalTie(std::string const &parameter, std::string const &tie);

  std::string removeTopIndex(std::string const &parameter) const;

  std::string m_parameter;
  std::string m_tie;
};

} // namespace MantidWidgets
} // namespace MantidQt
