// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FittingGlobals.h"

namespace MantidQt {
namespace MantidWidgets {

GlobalTie::GlobalTie(std::string const &parameter, std::string const &tie)
    : m_parameter(parameter), m_tie(tie) {}

std::string GlobalTie::removeTopIndex(std::string const &parameter) const {
  auto result = parameter;
  auto const firstDotIndex = result.find(".");
  if (firstDotIndex != std::string::npos)
    result.erase(0, firstDotIndex + 1);
  return result;
}

} // namespace MantidWidgets
} // namespace MantidQt
