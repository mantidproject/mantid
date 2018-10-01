#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/Python/Sip.h"

#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace {
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
  if (subplotspec > 0)
    fig.addSubPlot(subplotspec);
  return createPyCanvasFromFigure(std::move(fig));
}
} // namespace

/**
 * @brief Common constructor code for FigureCanvasQt
 * @param cppCanvas A pointer to the FigureCanvasQt object
 */
void initLayout(FigureCanvasQt *cppCanvas) {
  cppCanvas->setLayout(new QVBoxLayout());
  QWidget *pyCanvas = Python::extract<QWidget>(cppCanvas->pyobj());
  cppCanvas->layout()->addWidget(pyCanvas);
  pyCanvas->setMouseTracking(false);
  pyCanvas->installEventFilter(cppCanvas);
}

/**
 * @brief Constructor specifying an axes subplot spec and optional parent.
 * An Axes with the given subplot specification is created on construction
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.figure.Figure.html?highlight=add_subplot#matplotlib.figure.Figure.add_subplot
 * @param subplotspec A matplotlib subplot spec defined as a 3-digit integer
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(int subplotspec, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvas(subplotspec), "draw"),
      m_figure(Figure(Python::Object(pyobj().attr("figure")))) {
  // Cannot use delegating constructor here as InstanceHolder needs to be
  // initialized before the axes can be created
  initLayout(this);
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
  initLayout(this);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
