#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"
#include "MantidQtWidgets/MplCpp/Python/Sip.h"

#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @return A new Figure instance
 */
Python::Object figure(bool tightLayout = true) {
  auto fig = Python::NewRef(PyImport_ImportModule("matplotlib.figure"))
                 .attr("Figure")();
  if (tightLayout) {
    auto kwargs = Python::NewRef(Py_BuildValue("{s:f}", "pad", 0.5));
    fig.attr("set_tight_layout")(kwargs);
  }
  return fig;
}

/**
 * @return A new FigureCanvasQT object
 */
Python::Object createPyCanvas() {
  return backendModule().attr("FigureCanvasQTAgg")(figure());
}

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
    : QWidget(parent), InstanceHolder(createPyCanvas(), "draw"),
      m_axes(pyobj().attr("figure").attr("add_subplot")(subplotspec)) {
  // Cannot use delegating constructor here as InstanceHolder needs to be
  // initialized before the axes can be created
  initLayout(this);
}

/**
 * @brief Constructor specifying an existing axes object and optional
 * parent.
 * @param axes An existing axes instance
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(Axes axes, QWidget *parent)
    : QWidget(parent), InstanceHolder(createPyCanvas(), "draw"),
      m_axes(std::move(axes)) {
  initLayout(this);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
