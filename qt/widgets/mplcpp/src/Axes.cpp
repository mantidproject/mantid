#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/ErrorHandling.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

using Mantid::PythonInterface::Converters::VectorToNDArray;
using Mantid::PythonInterface::Converters::WrapReadOnly;
using Mantid::PythonInterface::PythonRuntimeError;

/**
 * Construct an Axes wrapper around an existing Axes instance
 * @param obj A matplotlib.axes.Axes instance
 */
Axes::Axes(Python::Object obj) : InstanceHolder(std::move(obj), "plot") {}

/**
 * @brief Set the X-axis label
 * @param label String for the axis label
 */
void Axes::setXLabel(const char *label) { pyobj().attr("set_xlabel")(label); }

/**
 * @brief Set the Y-axis label
 * @param label String for the axis label
 */
void Axes::setYLabel(const char *label) { pyobj().attr("set_ylabel")(label); }

/**
 * @brief Set the title
 * @param label String for the title label
 */
void Axes::setTitle(const char *label) { pyobj().attr("set_title")(label); }

/**
 * @brief Take the data and draw a single Line2D on the axes
 * @param xdata A vector containing the X data
 * @param ydata A vector containing the Y data
 * @param format A format string accepted by matplotlib.axes.Axes.plot. The
 * default is 'b-'
 * @return A new Line2D object
 */
Line2D Axes::plot(std::vector<double> xdata, std::vector<double> ydata,
                  const char *format) {
  auto throwIfEmpty = [](const std::vector<double> &data, char vecId) {
    if (data.empty()) {
      throw std::invalid_argument(
          std::string("Cannot plot line. Empty vector=") + vecId);
    }
  };
  throwIfEmpty(xdata, 'X');
  throwIfEmpty(ydata, 'Y');

  // Wrap the vector data in a numpy facade to avoid a copy.
  // The vector still owns the data so it needs to be kept alive too
  VectorToNDArray<double, WrapReadOnly> wrapNDArray;
  auto xarray{Python::NewRef(wrapNDArray(xdata))},
      yarray{Python::NewRef(wrapNDArray(ydata))};

  try {
    return Line2D{pyobj().attr("plot")(xarray, yarray, format)[0],
                  std::move(xdata), std::move(ydata)};

  } catch (Python::ErrorAlreadySet &) {
    throw PythonRuntimeError();
  }
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
