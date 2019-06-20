// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/PeakMarker.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

using Mantid::PythonInterface::PythonException;

namespace {

Python::Object
newMarker(FigureCanvasQt *canvas, int peakID, double x, double yTop,
          double yBottom, double fwhm,
          boost::optional<QHash<QString, QVariant>> const &otherKwargs) {
  GlobalInterpreterLock lock;

  Python::Object markersModule{Python::NewRef(
      PyImport_ImportModule("mantidqt.widgets.fitpropertybrowser.markers"))};

  auto const args = Python::NewRef(Py_BuildValue(
      "(Oidddd)", canvas->pyobj().ptr(), peakID, x, yTop, yBottom, fwhm));
  Python::Dict kwargs = Python::qHashToDict(otherKwargs.get());

  auto const marker = markersModule.attr("PeakMarker")(*args, **kwargs);
  return marker;
}

} // namespace

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Create a PeakMarker instance
 */
PeakMarker::PeakMarker(FigureCanvasQt *canvas, int peakID, double x,
                       double yTop, double yBottom, double fwhm,
                       QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(
          newMarker(canvas, peakID, x, yTop, yBottom, fwhm, otherKwargs)) {}

/**
 * @brief Redraw the PeakMarker
 */
void PeakMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

/**
 * @brief Removes the PeakMarker from the canvas
 */
void PeakMarker::remove() { callMethodNoCheck<void>(pyobj(), "remove"); }

/**
 * @brief Updates the centre, height and fwhm for the peak.
 * @param centre The new centre.
 * @param height The new height.
 * @param fwhm The new height.
 */
void PeakMarker::updatePeak(double centre, double height, double fwhm) {
  callMethodNoCheck<void>(pyobj(), "update_peak", centre, height, fwhm);
}

/**
 * @brief Get the centre, height and fwhm of the peak.
 * @return A tuple containing the centre, height and fwhm of the peak
 */
std::tuple<double, double, double> PeakMarker::peakProperties() const {
  GlobalInterpreterLock lock;

  auto const toDouble = [](Python::Object const &value) {
    return PyFloat_AsDouble(value.ptr());
  };

  auto const properties = pyobj().attr("peak_properties")();
  return std::make_tuple<double, double, double>(toDouble(properties[0]),
                                                 toDouble(properties[1]),
                                                 toDouble(properties[2]));
}

/**
 * @brief Checks if the centre marker is being moved.
 * @return True if the centre peak is being moved.
 */
bool PeakMarker::isMoving() const {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("is_moving")());
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

/**
 * @brief Selects the PeakMarker.
 */
void PeakMarker::select() { callMethodNoCheck<void>(pyobj(), "select"); }

/**
 * @brief Deselects the PeakMarker.
 */
void PeakMarker::deselect() { callMethodNoCheck<void>(pyobj(), "deselect"); }

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 */
void PeakMarker::mouseMoveStart(double x, double y) {
  callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y, false);
}

/**
 * @brief Notifies the relevant marker to stop moving.
 */
void PeakMarker::mouseMoveStop() {
  callMethodNoCheck<void>(pyobj(), "mouse_move_stop");
}

/**
 * @brief Notifies the relevant marker to start moving.
 * @param x The x position of the mouse press in axes coords.
 * @param y The y position of the mouse press in axes coords.
 * @return True if one of the VerticalMarker's within the RangeMarker has been
 * moved.
 */
bool PeakMarker::mouseMove(double x, double y) {
  GlobalInterpreterLock lock;

  auto const movedPy = Python::Object(pyobj().attr("mouse_move")(x, y, false));
  return PyLong_AsLong(movedPy.ptr()) > 0;
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
