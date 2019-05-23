// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidPythonInterface/core/CallMethod.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::callMethodNoCheck;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
Python::Object newFigure(bool tightLayout = true) {
  GlobalInterpreterLock lock;
  Python::Object figureModule{
      Python::NewRef(PyImport_ImportModule("matplotlib.figure"))};
  auto fig = figureModule.attr("Figure")();
  if (tightLayout) {
    auto tight = Python::NewRef(Py_BuildValue("{sf}", "pad", 0.5));
    fig.attr("set_tight_layout")(tight);
  }
  return fig;
}
} // namespace

/**
 * Construct a C++ wrapper around an existing figure instance
 * @param obj An existing Figure instance
 */
Figure::Figure(Python::Object obj)
    : Python::InstanceHolder(std::move(obj), "add_axes") {}

/**
 * Construct a new default figure.
 * @param tightLayout If true set a tight layout on the matplotlib figure
 */
Figure::Figure(bool tightLayout)
    : Python::InstanceHolder(newFigure(tightLayout)) {}

/**
 * Reset the background color of the figure.
 * @param color A character string indicating the color.
 * See https://matplotlib.org/api/colors_api.html
 */
void Figure::setFaceColor(const char *color) {
  callMethodNoCheck<void, const char *>(pyobj(), "set_facecolor", color);
}

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
  GlobalInterpreterLock lock;
  return Axes{pyobj().attr("add_axes")(
      Python::NewRef(Py_BuildValue("(ffff)", left, bottom, width, height)))};
}

/**
 * Add a subplot Axes to the figure
 * @param subplotspec
 * @return
 */
Axes Figure::addSubPlot(int subplotspec) {
  GlobalInterpreterLock lock;
  return Axes{pyobj().attr("add_subplot")(subplotspec)};
}

/**
 * @brief Add a colorbar to this figure
 * @param mappable An objet providing the mapping of data to rgb colors
 * @param cax An axes instance to hold the color bar
 * @param ticks An optional array or ticker.Locator object to control tick
 * placement
 * @param format An optional object describing how to format the tick labels
 * @return A reference to the matplotlib.colorbar.Colorbar object
 */
Python::Object Figure::colorbar(const ScalarMappable &mappable, const Axes &cax,
                                const Python::Object &ticks,
                                const Python::Object &format) {
  GlobalInterpreterLock lock;
  auto args = Python::NewRef(
      Py_BuildValue("(OO)", mappable.pyobj().ptr(), cax.pyobj().ptr()));
  auto kwargs = Python::NewRef(
      Py_BuildValue("{sOsO}", "ticks", ticks.ptr(), "format", format.ptr()));
  Python::Object attr{pyobj().attr("colorbar")};
  return Python::NewRef(PyObject_Call(attr.ptr(), args.ptr(), kwargs.ptr()));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
