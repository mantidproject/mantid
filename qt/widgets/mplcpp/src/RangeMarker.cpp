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
 * @param colour The color of the range marker
 * @param x_minimum The x coordinate of the minimum marker
 * @param x_maximum The x coordinate of the maximum marker
 */
RangeMarker::RangeMarker(FigureCanvasQt *canvas, QString const &color,
                         double x_minimum, double x_maximum,
                         QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(
          newMarker(canvas, color, x_minimum, x_maximum, otherKwargs)) {}

/**
 * @brief Redraw the RangeMarker
 */
void RangeMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

/**
 * @brief Remove the RangeMarker from the plot
 */
void RangeMarker::remove() { callMethodNoCheck<void>(pyobj(), "remove"); }

/**
 * @brief Sets the color of the RangeMarker.
 * @param color The color to set the RangeMarker to.
 */
void RangeMarker::setColor(QString const &color) {
  callMethodNoCheck<void>(pyobj(), "set_color", color.toLatin1().constData());
}

/**
 * @brief Sets the x range for the RangeMarker.
 * @param minimum The minimum of the range.
 * @param maximum The maximum of the range.
 */
void RangeMarker::setXRange(double minimum, double maximum) {
  callMethodNoCheck<void>(pyobj(), "set_x_range", minimum, maximum);
}

/**
 * @brief Gets the x range for the RangeMarker.
 * @return A tuple containing the minimum and maximum of the range.
 */
std::tuple<double, double> RangeMarker::getXRange() const {
  GlobalInterpreterLock lock;

  auto const toDouble = [](Python::Object const &value) {
    return PyFloat_AsDouble(value.ptr());
  };

  auto const coords = pyobj().attr("get_x_range")();
  return std::make_tuple<double, double>(toDouble(coords[0]),
                                         toDouble(coords[1]));
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 */
void RangeMarker::mouseMoveStart(double x, double y) {
  callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y, false);
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in pixels.
 * @param y The y position of the mouse press in pixels.
 */
void RangeMarker::mouseMoveStart(int x, int y) {
  callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y, true);
}

/**
 * @brief Notifies the relevant marker to stop moving.
 */
void RangeMarker::mouseMoveStop() {
  callMethodNoCheck<void>(pyobj(), "mouse_move_stop");
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 * @return True if one of the VerticalMarker's within the RangeMarker has been
 * moved.
 */
bool RangeMarker::mouseMove(double x, double y) {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("mouse_move")(x, y, false));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in pixels.
 * @param y The y position of the mouse press in pixels.
 * @return True if one of the VerticalMarker's within the RangeMarker has been
 * moved.
 */
bool RangeMarker::mouseMove(int x, int y) {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("mouse_move")(x, y, true));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
