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

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
