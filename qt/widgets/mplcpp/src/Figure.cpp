#include "MantidQtWidgets/MplCpp/Figure.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object newFigure(bool tightLayout = true) {
  Python::Object figureModule{
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

/**
 * Add an Axes of the given dimensions to the current figure
 * All quantities are in fractions of figure width and height
 * @param left The X coordinate of the lower-left corner
 * @param bottom The Y coordinate of the lower-left corner
 * @param width The width of the Axes
 * @param height The heigh of the Axes
 * @return A new Axes instance
 */
Axes Figure::addAxes(double left, double bottom, double width, double height) {
  return Axes{pyobj().attr("add_axes")(
      Python::NewRef(Py_BuildValue("(ffff)", left, bottom, width, height)))};
}

/**
 * Add a subplot Axes to the figure
 * @param subplotspec
 * @return
 */
Axes Figure::addSubPlot(int subplotspec) {
  return Axes{pyobj().attr("add_subplot")(subplotspec)};
}

/**
 * @brief Add a colorbar to this figure
 * @param mappable An objet providing the mapping of data to rgb colors
 * @param cax An axes instance to hold the color bar
 * @return A reference to the matplotlib.colorbar.Colorbar object
 */
Python::Object Figure::colorbar(ScalarMappable mappable, Axes cax) {
  return Python::Object(
      pyobj().attr("colorbar")(mappable.pyobj(), cax.pyobj()));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
