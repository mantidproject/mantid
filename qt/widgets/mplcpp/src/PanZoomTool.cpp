// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/PanZoomTool.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::callMethodNoCheck;

namespace MantidQt {
namespace Widgets {
namespace Python = Common::Python;
namespace MplCpp {

namespace {
// Name of the qt based tool bar that will be created
// to access the navigation facilities
constexpr auto TOOLBAR_CLS = "NavigationToolbar2QT";

// This implementation is tied to the implementation
// details of NavigationToolbar in matplotlib.backend_bases
// It relies on the values of strings such as the mode
// and the name of the the zoom/pan methods
constexpr auto TOOLBAR_MODE_ATTR = "mode";
constexpr auto TOOLBAR_ZOOM_MODE_STR = "zoom rect";
constexpr auto TOOLBAR_ZOOM_METHOD = "zoom";
constexpr auto TOOLBAR_PAN_MODE_STR = "pan/zoom";
constexpr auto TOOLBAR_PAN_METHOD = "pan";

/// Return the matplotlib NavigationToolbar type appropriate
/// for our backend. It is returned hidden
Python::Object mplNavigationToolbar(FigureCanvasQt *canvas) {
  const auto backend = backendModule();
  bool showCoordinates(false);
  auto obj = Python::Object(backend.attr(TOOLBAR_CLS)(
      canvas->pyobj(), canvas->pyobj(), showCoordinates));
  obj.attr("hide")();
  return obj;
}

} // namespace

/**
 * Create a object to attach pan/zoom capability to the given canvas.
 * @param canvas A reference to an existing FigureCanvasQt object
 */
PanZoomTool::PanZoomTool(FigureCanvasQt *canvas)
    : Python::InstanceHolder(mplNavigationToolbar(canvas)), m_canvas(canvas) {}

/**
 *
 * @return True if the zoom is enabled, false otherwise
 */
bool PanZoomTool::isZoomEnabled() const {
  GlobalInterpreterLock lock;
  return (pyobj().attr(TOOLBAR_MODE_ATTR) == TOOLBAR_ZOOM_MODE_STR);
}

/**
 * Enable/disable zoom mode
 * @param on If true enable zoom mode else disable it
 */
void PanZoomTool::enableZoom(const bool requestOn) {
  // The base functionality works as a toggle
  const bool isOn = isZoomEnabled();
  if ((requestOn && !isOn) || (!requestOn && isOn)) {
    callMethodNoCheck<void>(pyobj(), TOOLBAR_ZOOM_METHOD);
  }
}

/**
 * Resets the view to encompass all of the data
 */
void PanZoomTool::zoomOut() {
  GlobalInterpreterLock lock;
  m_canvas->gca().autoscale(true);
  m_canvas->drawIdle();
}

/**
 *
 * @return True if the pan is enabled, false otherwise
 */
bool PanZoomTool::isPanEnabled() const {
  GlobalInterpreterLock lock;
  return (pyobj().attr(TOOLBAR_MODE_ATTR) == TOOLBAR_PAN_MODE_STR);
}

/**
 * Enable/disable pan mode
 * @param on If true enable pan mode else disable it
 */
void PanZoomTool::enablePan(const bool on) {
  // The base functionality works as a toggle
  const bool isOn = isPanEnabled();
  if ((on && !isOn) || (!on && isOn)) {
    callMethodNoCheck<void>(pyobj(), TOOLBAR_PAN_METHOD);
  }
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
