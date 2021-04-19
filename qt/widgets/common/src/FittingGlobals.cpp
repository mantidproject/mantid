// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FittingGlobals.h"

namespace MantidQt {
namespace MantidWidgets {

GlobalParameter::GlobalParameter(std::string const &parameter) : m_parameter(parameter) {}

GlobalTie::GlobalTie(std::string const &parameter, std::string const &tie) : m_parameter(parameter), m_tie(tie) {}

std::string GlobalTie::toCompositeParameter(std ::string const &fullParameter) const {
  auto const dotIndex = fullParameter.find(".");
  if (dotIndex != std::string::npos) {
    return fullParameter.substr(0, dotIndex + 1) + "f0." + fullParameter.substr(dotIndex + 1);
  }
  return fullParameter;
}

std::string GlobalTie::toNonCompositeParameter(std ::string const &fullParameter) const {
  auto const firstDotIndex = fullParameter.find(".");
  if (firstDotIndex != std::string::npos) {
    auto const secondDotIndex = fullParameter.find(".", firstDotIndex + 1);
    if (secondDotIndex != std::string::npos) {
      return fullParameter.substr(0, firstDotIndex + 1) + fullParameter.substr(secondDotIndex + 1);
    }
  }
  return fullParameter;
}

} // namespace MantidWidgets
} // namespace MantidQt
