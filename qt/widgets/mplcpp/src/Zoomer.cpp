// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Zoomer.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
const char *TOOLBAR_CLS = "NavigationToolbar2QT";
const char *TOOLBAR_MODE_ATTR = "mode";
const char *TOOLBAR_MODE_ZOOM = "zoom rect";

/// Return the matplotlib NavigationToolbar type appropriate
/// for our backend. It is returned hidden
Python::Object mplNavigationToolbar(FigureCanvasQt *canvas) {
  auto backend = backendModule();
  bool showCoordinates(false);
  auto obj = Python::Object(backend.attr(TOOLBAR_CLS)(
      canvas->pyobj(), canvas->pyobj(), showCoordinates));
  obj.attr("hide")();
  return obj;
}

} // namespace

/**
 * Create a Zoomer object to attach zooming capability to
 * the given canvas.
 * @param canvas A reference to an existing FigureCanvasQt object
 */
Zoomer::Zoomer(FigureCanvasQt *canvas)
    : Python::InstanceHolder(mplNavigationToolbar(canvas)), m_canvas(canvas) {}

/**
 *
 * @return True if zooming has been enabled, false otherwise
 */
bool Zoomer::isZoomEnabled() const {
  return (pyobj().attr(TOOLBAR_MODE_ATTR) == TOOLBAR_MODE_ZOOM);
}

/**
 * Enable/disable zooming mode
 */
void Zoomer::enableZoom(bool requestOn) {
  // The base functionality works as a toggle
  const bool isOn = isZoomEnabled();
  if ((requestOn && !isOn) || (!requestOn && isOn)) {
    pyobj().attr("zoom")();
  }
}

/**
 * Resets the view to encompass all of the data
 */
void Zoomer::zoomOut() {
  m_canvas->gca().autoscale(true);
  m_canvas->drawIdle();
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
