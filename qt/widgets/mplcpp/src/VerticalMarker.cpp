// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/VerticalMarker.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/Common/Python/QHashToDict.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
// using Mantid::PythonInterface::PythonException;
using Mantid::PythonInterface::callMethodNoCheck;
using namespace MantidQt::Widgets::Common;
using namespace MantidQt::Widgets::MplCpp;

namespace {

Python::Object
newMarker(FigureCanvasQt *canvas, QString const &colour, double const &x,
          boost::optional<QHash<QString, QVariant>> const &otherKwargs) {
  GlobalInterpreterLock lock;

  Python::Object markersModule{Python::NewRef(
      PyImport_ImportModule("mantidqt.widgets.fitpropertybrowser.markers"))};

  auto const args = Python::NewRef(Py_BuildValue(
      "(Osd)", canvas->pyobj().ptr(), colour.toLatin1().constData(), x));
  Python::Dict kwargs = Python::qHashToDict(otherKwargs.get());

  auto const marker = markersModule.attr("VerticalMarker")(*args, **kwargs);
  return marker;
}

} // namespace

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief Create a VerticalMarker instance
 * @param canvas The canvas to draw the vertical marker on to
 * @param colour The colour of the vertical marker
 * @param x The x coordinate of the marker
 */
VerticalMarker::VerticalMarker(FigureCanvasQt *canvas, QString const &colour,
                               double const &x,
                               QHash<QString, QVariant> const &otherKwargs)
    : InstanceHolder(newMarker(canvas, colour, x, otherKwargs)) {}

void VerticalMarker::redraw() { callMethodNoCheck<void>(pyobj(), "redraw"); }

void VerticalMarker::remove() { callMethodNoCheck<void>(pyobj(), "remove"); }

void VerticalMarker::setColor(QString const &color) {
  callMethodNoCheck<void>(pyobj(), "set_color", color.toLatin1().constData());
}

void VerticalMarker::mouseMoveStart(double x, double y) {
  callMethodNoCheck<void>(pyobj(), "mouse_move_start", x, y);
}

void VerticalMarker::mouseMoveEnd() {
  callMethodNoCheck<void>(pyobj(), "mouse_move_stop");
}

void VerticalMarker::mouseMove(double x) {
  callMethodNoCheck<void>(pyobj(), "mouse_move", x);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
