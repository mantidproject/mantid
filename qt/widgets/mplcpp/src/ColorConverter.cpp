// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/ColorConverter.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object colorConverter() {
  auto colors(Python::NewRef(PyImport_ImportModule("matplotlib.colors")));
  return Python::Object(colors.attr("colorConverter"));
}

/// Convert a Python float to a byte value from 0->255
inline int toByte(const Python::Object &pyfloat) {
  const double rgbFloat = PyFloat_AsDouble(pyfloat.ptr()) * 255;
  return static_cast<int>(rgbFloat);
}
} // namespace

/**
 * @brief Convert a matplotlib color specification to a QColor object
 * @param colorSpec A matplotlib color spec. See
 * https://matplotlib.org/api/colors_api.html?highlight=colors#module-matplotlib.colors
 * @return A QColor object that represents this color
 */
QColor MantidQt::Widgets::MplCpp::ColorConverter::toRGB(
    const Python::Object &colorSpec) {
  GlobalInterpreterLock lock;
  auto tuple = Python::Object(colorConverter().attr("to_rgb")(colorSpec));
  return QColor(toByte(tuple[0]), toByte(tuple[1]), toByte(tuple[2]));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
