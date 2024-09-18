// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/Common/Python/Sip.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/Figure.h"

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/NDArray.h"

#include <QVBoxLayout>

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::NDArray;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {
namespace {

const char *DEFAULT_FACECOLOR = "w";

/**
 * @param fig An existing matplotlib Figure instance
 * @return A new FigureCanvasQT object
 */
Python::Object createPyCanvasFromFigure(const Figure &fig) {
  GlobalInterpreterLock lock;
  return backendModule().attr("FigureCanvasQTAgg")(fig.pyobj());
}

/**
 * @param subplotspec A matplotlib subplot spec defined as a 3-digit
 * integer.
 * @param projection A string denoting the projection to use
 * @return A new FigureCanvasQT object
 */
Python::Object createPyCanvas(const int subplotspec, const QString &projection) {
  Figure fig{true};
  fig.setFaceColor(DEFAULT_FACECOLOR);

  if (subplotspec > 0)
    fig.addSubPlot(subplotspec, projection);
  return createPyCanvasFromFigure(fig);
}
} // namespace

/**
 * @brief Common constructor code for FigureCanvasQt
 * @param cppCanvas A pointer to the FigureCanvasQt object
 * @return A pointer to the C++ widget extract from the Python FigureCanvasQT
 * object
 */
QWidget *initLayout(FigureCanvasQt *cppCanvas) {
  cppCanvas->setLayout(new QVBoxLayout());
  QWidget *pyCanvas = Python::extract<QWidget>(cppCanvas->pyobj());
  cppCanvas->layout()->addWidget(pyCanvas);
  return pyCanvas;
}

/**
 * @brief Constructor specifying an axes subplot spec and optional parent.
 * An Axes with the given subplot specification is created on construction
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.figure.Figure.html?highlight=add_subplot#matplotlib.figure.Figure.add_subplot
 * @param subplotspec A matplotlib subplot spec defined as a 3-digit integer
 * @param projection A string denoting the projection to use on the canvas
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(const int subplotspec, const QString &projection, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvas(subplotspec, projection), "draw") {
  // Cannot use delegating constructor here as we have to ensure that we have acquired the GIL
  // before creating the Figure...
  GlobalInterpreterLock lock;
  m_figure = Figure(Python::Object(pyobj().attr("figure")));
  // ... and the InstanceHolder needs to be initialized before the axes can be created
  m_mplCanvas = initLayout(this);
}

/**
 * @brief Constructor specifying an existing axes object and optional
 * parent.
 * @param fig An existing figure instance containing an axes
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(Figure fig, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvasFromFigure(fig), "draw"), m_figure(std::move(fig)) {
  m_mplCanvas = initLayout(this);
}

/**
 * Allows events destined for the child canvas to be intercepted by a filter
 * object. The Matplotlib canvas defines its own event handlers for certain
 * events but FigureCanvasQt does not directly inherit from the matplotlib
 * canvas so we cannot use the standard virtual event methods to intercept them.
 * Instead the event filter allows an object to capture and process them.
 * @param filter A pointer to an object overriding the eventFilter method
 */
void FigureCanvasQt::installEventFilterToMplCanvas(QObject *filter) {
  assert(m_mplCanvas);
  m_mplCanvas->installEventFilter(filter);
}

/**
 * Sets how tight_layout is called when drawing. ("pad", "w_pad", "h_pad",
 * "rect", etc.)
 * @param args A hash of parameters to pass to set_tight_layout
 */
void FigureCanvasQt::setTightLayout(QHash<QString, QVariant> const &args) { m_figure.setTightLayout(args); }

/**
 * @param pos A point in Qt screen coordinates from, for example, a mouse click
 * @return A QPointF defining the position in data coordinates
 */
QPointF FigureCanvasQt::toDataCoords(QPoint pos) const {
  // There is no isolated method for doing the transform on matplotlib's
  // classes. The functionality is bound up inside other methods
  // so we are forced to duplicate the behaviour here.
  // The following code is derived form what happens in
  // matplotlib.backends.backend_qt5.FigureCanvasQT &
  // matplotlib.backend_bases.LocationEvent where we transform first to
  // matplotlib's coordinate system, (0,0) is bottom left,
  // and then to the data coordinates
  GlobalInterpreterLock lock;
  const int dpiRatio(static_cast<int>(
      PyFloat_AsDouble(Python::Object(m_figure.pyobj().attr("canvas").attr("device_pixel_ratio")).ptr())));
  const double xPosPhysical = pos.x() * dpiRatio;
  // Y=0 is at the bottom
  double height = PyFloat_AsDouble(Python::Object(m_figure.pyobj().attr("bbox").attr("height")).ptr());
  const double yPosPhysical = ((height / dpiRatio) - pos.y()) * dpiRatio;
  // Transform to data coordinates
  QPointF dataCoords;
  try {
    auto invTransform = gca().pyobj().attr("transData").attr("inverted")();
    NDArray transformed =
        invTransform.attr("transform_point")(Python::NewRef(Py_BuildValue("(ff)", xPosPhysical, yPosPhysical)));
    auto rawData = reinterpret_cast<double *>(transformed.get_data());
    dataCoords = QPointF(static_cast<qreal>(rawData[0]), static_cast<qreal>(rawData[1]));
  } catch (Python::ErrorAlreadySet &) {
    PyErr_Clear();
    // an exception indicates no transform possible. Matplotlib sets this as
    // an empty data coordinate so we will do the same
  }
  return dataCoords;
}

} // namespace MantidQt::Widgets::MplCpp
