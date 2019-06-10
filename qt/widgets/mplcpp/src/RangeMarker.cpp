// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/RangeMarker.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace {

Python::Object
newMarker(FigureCanvasQt *canvas, QString const &colour, double x_minimum,
          double x_maximum,
          boost::optional<QHash<QString, QVariant>> const &otherKwargs) {
  GlobalInterpreterLock lock;

  Python::Object markersModule{Python::NewRef(
      PyImport_ImportModule("mantidqt.widgets.fitpropertybrowser.markers"))};

  auto const args = Python::NewRef(
      Py_BuildValue("(Osdd)", canvas->pyobj().ptr(),
                    colour.toLatin1().constData(), x_minimum, x_maximum));
  Python::Dict kwargs = Python::qHashToDict(otherKwargs.get());

  auto const marker = markersModule.attr("RangeMarker")(*args, **kwargs);
  return marker;
}

} // namespace

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Create a RangeMarker instance
 * @param canvas The canvas to draw the range marker on to
 * @param colour The colour of the vertical marker
 * @param x_minimum The x coordinate of the minimum marker
 * @param x_maximum The x coordinate of the maximum marker
 */
RangeMarker::RangeMarker(FigureCanvasQt *canvas, QString const &colour,
                         double x_minimum, double x_maximum,
                         QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(
          newMarker(canvas, colour, x_minimum, x_maximum, otherKwargs)) {}

void RangeMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

void RangeMarker::remove() { callMethodNoCheck<void>(pyobj(), "remove"); }

void RangeMarker::setColor(QString const &color) {
  callMethodNoCheck<void>(pyobj(), "set_color", color.toLatin1().constData());
}

void RangeMarker::setXRange(double minimum, double maximum) {
  callMethodNoCheck<void>(pyobj(), "set_x_range", minimum, maximum);
}

std::tuple<double, double> RangeMarker::getXRange() {
  GlobalInterpreterLock lock;

  auto const toDouble = [](Python::Object const &value) {
    return PyFloat_AsDouble(value.ptr());
  };

  auto const coords = pyobj().attr("get_x_range")();
  return std::make_tuple<double, double>(toDouble(coords[0]),
                                         toDouble(coords[1]));
}

void RangeMarker::mouseMoveStart(double x, double y, bool usingPixels) {
  callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y, usingPixels);
}

void RangeMarker::mouseMoveStop() {
  callMethodNoCheck<void>(pyobj(), "mouse_move_stop");
}

bool RangeMarker::mouseMove(double x, double y, bool usingPixels) {
  GlobalInterpreterLock lock;

  auto const movedPy =
      Python::Object(pyobj().attr("mouse_move")(x, y, usingPixels));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
