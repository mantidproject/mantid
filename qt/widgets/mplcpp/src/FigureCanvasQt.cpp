#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/Python/Sip.h"

#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace {

const char *DEFAULT_FACECOLOR = "w";

/**
 * @param fig An existing matplotlib Figure instance
 * @return A new FigureCanvasQT object
 */
Python::Object createPyCanvasFromFigure(Figure fig) {
  return backendModule().attr("FigureCanvasQTAgg")(fig.pyobj());
}

/**
 * @param subplotspec A matplotlib subplot spec defined as a 3-digit
 * integer.
 * @return A new FigureCanvasQT object
 */
Python::Object createPyCanvas(int subplotspec) {
  Figure fig{true};
  fig.setFaceColor(DEFAULT_FACECOLOR);

  if (subplotspec > 0)
    fig.addSubPlot(subplotspec);
  return createPyCanvasFromFigure(std::move(fig));
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
 * @param facecolor String denoting the figure's facecolor
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(int subplotspec, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvas(subplotspec), "draw"),
      m_figure(Figure(Python::Object(pyobj().attr("figure")))) {
  // Cannot use delegating constructor here as InstanceHolder needs to be
  // initialized before the axes can be created
  m_mplCanvas = initLayout(this);
}

/**
 * @brief Constructor specifying an existing axes object and optional
 * parent.
 * @param fig An existing figure instance containing an axes
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(Figure fig, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvasFromFigure(fig), "draw"),
      m_figure(std::move(fig)) {
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

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
