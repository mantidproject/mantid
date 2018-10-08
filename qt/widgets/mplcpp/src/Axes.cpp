#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/VersionCompat.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

using Mantid::PythonInterface::Converters::VectorToNDArray;
using Mantid::PythonInterface::Converters::WrapReadOnly;
using Mantid::PythonInterface::PythonRuntimeError;

namespace {
/**
 * Create a QString from a Python string object
 * @param pystr A Python string object. This is not checked
 * @return A new QString
 */
QString toQString(const Python::Object &pystr) {
  return QString::fromLatin1(TO_CSTRING(pystr.ptr()));
}
} // namespace

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
 * @return A new Line2D object that owns the xdata, ydata vectors. If the
 * return value is not captured the line will be automatically removed from
 * the canvas as the vector data will be destroyed.
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
  // It is transferred to the Line2D for this purpose.
  VectorToNDArray<double, WrapReadOnly> wrapNDArray;
  Python::Object xarray{Python::NewRef(wrapNDArray(xdata))},
      yarray{Python::NewRef(wrapNDArray(ydata))};

  try {
    return Line2D{pyobj().attr("plot")(xarray, yarray, format)[0],
                  std::move(xdata), std::move(ydata)};

  } catch (Python::ErrorAlreadySet &) {
    throw PythonRuntimeError();
  }
}

/**
 * @brief Set the X-axis scale to the given value.
 * @param value New scale type. See
 * https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.set_xscale.html
 * @raises std::invalid_argument if the value is unknown
 */
void Axes::setXScale(const char *value) {
  try {
    pyobj().attr("set_xscale")(value);
  } catch (Python::ErrorAlreadySet &) {
    throw std::invalid_argument(std::string("setXScale: Unknown scale type ") +
                                value);
  }
}

/// @return The scale type of the X axis as a string
QString Axes::getXScale() const {
  return toQString(pyobj().attr("get_xscale")());
}

/**
 * @brief Set the Y-axis scale to the given value.
 * @param value New scale type. See
 * https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.set_xscale.html
 * @raises std::invalid_argument if the value is unknown
 */
void Axes::setYScale(const char *value) {
  try {
    pyobj().attr("set_yscale")(value);
  } catch (Python::ErrorAlreadySet &) {
    throw std::invalid_argument(std::string("setYScale: Unknown scale type ") +
                                value);
  }
}

/// @return The scale type of the Y axis as a string
QString Axes::getYScale() const {
  return toQString(pyobj().attr("get_yscale")());
}

/**
 * @brief Recompute the data limits from the current artists.
 * @param visibleOnly If true then only include visble artists in the
 * calculation
 */
void Axes::relim(bool visibleOnly) { pyobj().attr("relim")(visibleOnly); }

/**
 * Calls Axes.autoscale to enable/disable auto scaling
 * @param enable If true enable autoscaling and perform the automatic rescale
 */
void Axes::autoscale(bool enable) { pyobj().attr("autoscale")(enable); }

/**
 * Autoscale the view based on the current data limits. Calls
 * Axes.autoscale_view with the tight argument set to None. Autoscaling
 * must be turned on for this to work as expected
 * @param scaleX If true (default) scale the X axis limits
 * @param scaleY If true (default) scale the Y axis limits
 */
void Axes::autoscaleView(bool scaleX, bool scaleY) {
  pyobj().attr("autoscale_view")(Python::Object(), scaleX, scaleY);
}

/**
 * Autoscale the view based on the current data limits. Calls
 * Axes.autoscale_view
 * @param tight If true tight is False, the axis major locator will be used to
 * expand the view limits
 * @param scaleX If true (default) scale the X axis limits
 * @param scaleY If true (default) scale the Y axis limits
 */
void Axes::autoscaleView(bool tight, bool scaleX, bool scaleY) {
  pyobj().attr("autoscale_view")(tight, scaleX, scaleY);
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
