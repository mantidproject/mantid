// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/ParseNumerics.h"
#include <stdexcept>
namespace MantidQt {
namespace MantidWidgets {
double parseDouble(QString const &in) {
  static auto ok = false;
  auto out = in.toDouble(&ok);
  if (ok)
    return out;
  else
    throw std::runtime_error("Failed to parse '" + in.toStdString() + "' as a double numerical value.");
}

int parseDenaryInteger(QString const &in) {
  static auto ok = false;
  auto constexpr BASE = 10;
  auto out = in.toInt(&ok, BASE);
  if (ok)
    return out;
  else
    throw std::runtime_error("Failed to parse '" + in.toStdString() + "' as a denary integer.");
}
} // namespace MantidWidgets
} // namespace MantidQt
