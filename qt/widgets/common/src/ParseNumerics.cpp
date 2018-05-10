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
    throw std::runtime_error("Failed to parse '" + in.toStdString() +
                             "' as a double numerical value.");
}

int parseDenaryInteger(QString const &in) {
  static auto ok = false;
  auto constexpr BASE = 10;
  auto out = in.toInt(&ok, BASE);
  if (ok)
    return out;
  else
    throw std::runtime_error("Failed to parse '" + in.toStdString() +
                             "' as a denary integer.");
}
} // namespace MantidWidgets
} // namespace MantidQt
