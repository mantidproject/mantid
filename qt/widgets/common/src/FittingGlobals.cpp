// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FittingGlobals.h"

namespace MantidQt {
namespace MantidWidgets {

GlobalParameter::GlobalParameter(std::string const &parameter)
    : m_parameter(parameter) {}

GlobalTie::GlobalTie(std::string const &parameter, std::string const &tie)
    : m_parameter(parameter), m_tie(tie) {}

} // namespace MantidWidgets
} // namespace MantidQt
