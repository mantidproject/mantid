#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include "MantidQtWidgets/MplCpp/BackendQt.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @return A new Figure instance
 */
Python::Object figure() {
  return Python::NewRef(PyImport_ImportModule("matplotlib.figure"))
      .attr("Figure")();
}

/**
 * @return A new FigureCanvasQT object
 */
Python::Object createCanvas() {
  return backendModule().attr("FigureCanvasQT")(figure());
}

/**
 * @brief Default constructor with an optional parent widget. It defaults
 * the figure to contain a single subplot
 * @param parent The owning parent widget
 */
FigureCanvasQt::FigureCanvasQt(QWidget *parent)
    : QWidget(parent), InstanceHolder(createCanvas()),
      m_axes(pyobj().attr("figure").attr("add_subplot")(111)) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
