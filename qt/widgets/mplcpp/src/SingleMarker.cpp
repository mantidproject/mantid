// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/SingleMarker.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace {

Python::Object newMarker(FigureCanvasQt *canvas, QString const &colour, double position, double minimum, double maximum,
                         QString const &markerType, std::optional<QHash<QString, QVariant>> const &otherKwargs) {
  GlobalInterpreterLock lock;

  Python::Object markersModule{Python::NewRef(PyImport_ImportModule("mantidqt.plotting.markers"))};

  auto const args = Python::NewRef(Py_BuildValue("(Osddds)", canvas->pyobj().ptr(), colour.toLatin1().constData(),
                                                 position, minimum, maximum, markerType.toLatin1().constData()));
  Python::Dict kwargs = Python::qHashToDict(otherKwargs.value());

  auto const marker = markersModule.attr("SingleMarker")(*args, **kwargs);
  return marker;
}

} // namespace

namespace MantidQt::Widgets::MplCpp {

/**
 * @brief Create a SingleMarker instance
 * @param canvas The canvas to draw the marker on to
 * @param colour The color of the marker
 * @param position The axis position of the marker
 * @param minimum The lowest position the marker can reach
 * @param maximum The highest position the marker can reach
 * @param markerType Whether the marker is vertical or horizontal
 * @param otherKwargs Other optional kwargs
 */
SingleMarker::SingleMarker(FigureCanvasQt *canvas, QString const &color, double position, double minimum,
                           double maximum, QString const &markerType, QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(newMarker(canvas, color, position, minimum, maximum, markerType, otherKwargs)) {}

/**
 * @brief Redraw the marker
 */
void SingleMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

/**
 * @brief Remove the marker from the plot
 */
void SingleMarker::remove() {
  try {
    callMethodNoCheck<void>(pyobj(), "remove");
  } catch (PythonException const &) {
    // Marker has already been removed
  }
}

/**
 * @brief Sets the color of the marker.
 * @param color The color to set the marker to.
 */
void SingleMarker::setColor(QString const &color) {
  callMethodNoCheck<void>(pyobj(), "set_color", color.toLatin1().constData());
}

/**
 * @brief Sets the position of the marker.
 * @param position The markers new position.
 * @return True if the position has been changed.
 */
bool SingleMarker::setPosition(double position) {
  GlobalInterpreterLock lock;

  auto const positionChanged = Python::Object(pyobj().attr("set_position")(position));
  return PyLong_AsLong(positionChanged.ptr()) > 0;
}

/**
 * @brief Gets the axis position of the marker.
 * @return The position of the marker.
 */
double SingleMarker::getPosition() const {
  GlobalInterpreterLock lock;
  return PyFloat_AsDouble(pyobj().attr("get_position")().ptr());
}

/**
 * @brief Sets the upper and lower bounds of the marker.
 * @param minimum The lower bound.
 * @param maximum The upper bound.
 */
void SingleMarker::setBounds(double minimum, double maximum) {
  callMethodNoCheck<void>(pyobj(), "set_bounds", minimum, maximum);
}

/**
 * @brief Set the lower bound of the marker.
 * @param minimum The lower bound.
 */
void SingleMarker::setLowerBound(double minimum) { callMethodNoCheck<void>(pyobj(), "set_lower_bound", minimum); }

/**
 * @brief Set the upper bound of the marker.
 * @param maximum The upper bound.
 */
void SingleMarker::setUpperBound(double maximum) { callMethodNoCheck<void>(pyobj(), "set_upper_bound", maximum); }

/**
 * @brief Notifies the marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 */
void SingleMarker::mouseMoveStart(double x, double y) { callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y); }

/**
 * @brief Notifies the marker to stop moving.
 */
void SingleMarker::mouseMoveStop() { callMethodNoCheck<void>(pyobj(), "mouse_move_stop"); }

/**
 * @brief Notifies the marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 * @return True if the marker has been moved.
 */
bool SingleMarker::mouseMove(double x, double y) {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("mouse_move")(x, y));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

/**
 * @brief Returns true if the marker is moving.
 * @return True if the marker is being moved.
 */
bool SingleMarker::isMoving() {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("is_marker_moving")());
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

} // namespace MantidQt::Widgets::MplCpp
