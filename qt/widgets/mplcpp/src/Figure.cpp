#include "MantidQtWidgets/MplCpp/Figure.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object newFigure(bool tightLayout = true) {
  static auto figureModule{
      Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
  auto fig = figureModule.attr("Figure")();
  if (tightLayout) {
    auto kwargs = Python::NewRef(Py_BuildValue("{s:f}", "pad", 0.5));
    fig.attr("set_tight_layout")(kwargs);
  }
  return fig;
}
} // namespace

/**
 * Construct a new default figure.
 * @param tightLayout If true set a tight layout on the matplotlib figure
 */
Figure::Figure(bool tightLayout)
    : Python::InstanceHolder(newFigure(tightLayout)) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
