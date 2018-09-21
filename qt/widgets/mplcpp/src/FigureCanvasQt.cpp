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
Python::Object createCanvas() {
  return backendModule().attr("FigureCanvasQTAgg")(figure());
}

/**
 * @brief Default constructor with an optional parent widget. It defaults
 * the figure to contain a single subplot
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(QWidget *parent)
    : QWidget(parent), InstanceHolder(createCanvas(), "draw"),
      m_axes(this->pyobj().attr("figure").attr("add_subplot")(111)) {
  setLayout(new QVBoxLayout());
  QWidget *canvas = Python::extract<QWidget>(this->pyobj());
  layout()->addWidget(canvas);
  canvas->setMouseTracking(false);
  canvas->installEventFilter(this);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
