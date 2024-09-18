// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/RangeMarker.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace {

Python::Object newMarker(FigureCanvasQt *canvas, QString const &colour, double minimum, double maximum,
                         QString const &rangeType, std::optional<QHash<QString, QVariant>> const &otherKwargs) {
  GlobalInterpreterLock lock;

  Python::Object markersModule{Python::NewRef(PyImport_ImportModule("mantidqt.plotting.markers"))};

  auto const args = Python::NewRef(Py_BuildValue("(Osdds)", canvas->pyobj().ptr(), colour.toLatin1().constData(),
                                                 minimum, maximum, rangeType.toLatin1().constData()));
  Python::Dict kwargs = Python::qHashToDict(otherKwargs.value());

  auto const marker = markersModule.attr("RangeMarker")(*args, **kwargs);
  return marker;
}

} // namespace

namespace MantidQt::Widgets::MplCpp {

/**
 * @brief Create a RangeMarker instance
 * @param canvas The canvas to draw the range marker on to
 * @param colour The color of the range marker
 * @param minimum The coordinate of the minimum marker
 * @param maximum The coordinate of the maximum marker
 */
RangeMarker::RangeMarker(FigureCanvasQt *canvas, QString const &color, double minimum, double maximum,
                         QString const &rangeType, QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(newMarker(canvas, color, minimum, maximum, rangeType, otherKwargs)) {}

/**
 * @brief Redraw the RangeMarker
 */
void RangeMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

/**
 * @brief Remove the RangeMarker from the plot
 */
void RangeMarker::remove() {
  try {
    callMethodNoCheck<void>(pyobj(), "remove");
  } catch (PythonException const &) {
    // Marker has already been removed
  }
}

/**
 * @brief Sets the color of the RangeMarker.
 * @param color The color to set the RangeMarker to.
 */
void RangeMarker::setColor(QString const &color) {
  callMethodNoCheck<void>(pyobj(), "set_color", color.toLatin1().constData());
}

/**
 * @brief Sets the bounds of the RangeMarker.
 * @param lowerBound The lower bound.
 * @param upperBound The upper bound.
 */
void RangeMarker::setBounds(double lowerBound, double upperBound) {
  callMethodNoCheck<void>(pyobj(), "set_bounds", lowerBound, upperBound);
}

/**
 * @brief Sets the lower bound of the RangeMarker.
 * @param minimum The lower bound.
 */
void RangeMarker::setLowerBound(double lowerBound) { callMethodNoCheck<void>(pyobj(), "set_lower_bound", lowerBound); }

/**
 * @brief Sets the upper bound of the RangeMarker.
 * @param upperBound The upper bound.
 */
void RangeMarker::setUpperBound(double upperBound) { callMethodNoCheck<void>(pyobj(), "set_upper_bound", upperBound); }

/**
 * @brief Sets the range marked by the RangeMarker.
 * @param minimum The minimum of the range.
 * @param maximum The maximum of the range.
 */
void RangeMarker::setRange(double minimum, double maximum) {
  callMethodNoCheck<void>(pyobj(), "set_range", minimum, maximum);
}

/**
 * @brief Gets the range marked by the RangeMarker.
 * @return A tuple containing the minimum and maximum of the range.
 */
std::tuple<double, double> RangeMarker::getRange() const {
  GlobalInterpreterLock lock;

  auto const toDouble = [](Python::Object const &value) { return PyFloat_AsDouble(value.ptr()); };

  auto const coords = pyobj().attr("get_range")();
  return std::make_tuple<double, double>(toDouble(coords[0]), toDouble(coords[1]));
}

/**
 * @brief Sets the minimum the RangeMarker.
 * @param minimum The minimum of the range.
 */
void RangeMarker::setMinimum(double minimum) { callMethodNoCheck<void>(pyobj(), "set_minimum", minimum); }

/**
 * @brief Sets the minimum the RangeMarker.
 * @param minimum The minimum of the range.
 */
void RangeMarker::setMaximum(double maximum) { callMethodNoCheck<void>(pyobj(), "set_maximum", maximum); }

/**
 * @brief Gets minimum of the RangeMarker.
 * @return The minimum of the range.
 */
double RangeMarker::getMinimum() const {
  GlobalInterpreterLock lock;
  return PyFloat_AsDouble(pyobj().attr("get_minimum")().ptr());
}

/**
 * @brief Gets maximum of the RangeMarker.
 * @return The maximum of the range.
 */
double RangeMarker::getMaximum() const {
  GlobalInterpreterLock lock;
  return PyFloat_AsDouble(pyobj().attr("get_maximum")().ptr());
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 */
void RangeMarker::mouseMoveStart(double x, double y) { callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y); }

/**
 * @brief Notifies the relevant marker to stop moving.
 */
void RangeMarker::mouseMoveStop() { callMethodNoCheck<void>(pyobj(), "mouse_move_stop"); }

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 * @return True if one of the marker's within the RangeMarker has been
 * moved.
 */
bool RangeMarker::mouseMove(double x, double y) {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("mouse_move")(x, y));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

/**
 * @brief Returns true if one of the marker's is moving.
 * @return True if one of the marker's within the RangeMarker is being moved.
 */
bool RangeMarker::isMoving() {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("is_marker_moving")());
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

} // namespace MantidQt::Widgets::MplCpp
