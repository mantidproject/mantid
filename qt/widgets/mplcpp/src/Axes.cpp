// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidPythonInterface/core/CallMethod.h"
#include "MantidPythonInterface/core/Converters/VectorToNDArray.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/VersionCompat.h"

namespace MantidQt::Widgets::MplCpp {

using Mantid::PythonInterface::callMethodNoCheck;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using Mantid::PythonInterface::Converters::VectorToNDArray;
using Mantid::PythonInterface::Converters::WrapReadOnly;
using namespace MantidQt::Widgets::Common;

namespace {
/**
 * Create a QString from a Python string object
 * @param pystr A Python string object. This is not checked
 * @return A new QString
 */
QString toQString(const Python::Object &pystr) { return QString::fromLatin1(TO_CSTRING(pystr.ptr())); }

/**
 * Retrieve axes limits from an axes object as C++ tuple
 * @param axes A reference to the axes object
 * @param method The accessor name
 * @return A 2-tuple of the limits values
 */
std::tuple<double, double> limitsToTuple(const Python::Object &axes, const char *method) {
  auto toDouble = [](const Python::Object &value) { return PyFloat_AsDouble(value.ptr()); };
  auto limits = axes.attr(method)();
  return std::make_tuple<double, double>(toDouble(limits[0]), toDouble(limits[1]));
}

} // namespace

/**
 * Construct an Axes wrapper around an existing Axes instance
 * @param obj A matplotlib.axes.Axes instance
 */
Axes::Axes(Python::Object obj) : InstanceHolder(std::move(obj), "plot") {}

/**
 * Clear all artists from the axes
 */
void Axes::clear() { callMethodNoCheck<void>(pyobj(), "clear"); }

/**
 * Apply an operation to each artist in the given container
 * @param containerAttr The name of the container attribute
 * @param op An operation to apply to each artist in the contain
 */
void Axes::forEachArtist(const char *containerAttr, const ArtistOperation &op) {
  GlobalInterpreterLock lock;
  try {
    auto container = pyobj().attr(containerAttr);
    auto containerLength = Python::Len(container);
    for (decltype(containerLength) i = 0; i < containerLength; ++i) {
      op(Artist{container[i]});
    }
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

/**
 * Remove any artists in the container with a matching label. If none are found
 * then this is a no-op.
 * @param containerAttr The name of the container attribute
 * @param label The label of the artists to remove
 */
void Axes::removeArtists(const char *containerAttr, const QString &label) {
  GlobalInterpreterLock lock;
  const auto lineNameAsUnicode = Python::NewRef(PyUnicode_FromString(label.toLatin1().constData()));
  try {
    const auto container = pyobj().attr(containerAttr);
    auto containerLength = Python::Len(container);
    decltype(containerLength) index(0);
    while (index < containerLength) {
      Artist artist{container[index]};
      if (lineNameAsUnicode == artist.pyobj().attr("get_label")()) {
        artist.remove();
        containerLength = Python::Len(container);
      } else {
        index++;
      }
    }
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

/**
 * @brief Set the X-axis label
 * @param label String for the axis label
 */
void Axes::setXLabel(const char *label) { callMethodNoCheck<void, const char *>(pyobj(), "set_xlabel", label); }

/**
 * @brief Set the Y-axis label
 * @param label String for the axis label
 */
void Axes::setYLabel(const char *label) { callMethodNoCheck<void, const char *>(pyobj(), "set_ylabel", label); }

/**
 * @brief Set the title
 * @param label String for the title label
 */
void Axes::setTitle(const char *label) { callMethodNoCheck<void, const char *>(pyobj(), "set_title", label); }

/**
 * Format the tick labels on an axis or axes
 * @param char* axis :: [ 'x' | 'y' | 'both' ]
 * @param char* style :: [ 'sci' (or 'scientific') | 'plain' ] plain turns off
 * scientific notation
 * @param bool useOffset :: True, the offset will be
 * calculated as needed, False no offset will be used
 */
void Axes::tickLabelFormat(const std::string &axis, const std::string &style, const bool useOffset) {
  try {
    GlobalInterpreterLock lock;
    Python::List args;
    Python::Dict kwargs;
    kwargs["axis"] = axis;
    kwargs["style"] = style;
    kwargs["useOffset"] = useOffset;
    pyobj().attr("ticklabel_format")(*args, **kwargs);
  } catch (...) {
    throw PythonException();
  }
}

/**
 * (Re-)generate a legend on the axes
 * @param draggable If true the legend will be draggable
 * @return An artist object representing the legend
 */
Artist Axes::legend(const bool draggable) {
  GlobalInterpreterLock lock;
  Artist legend{pyobj().attr("legend")()};
  const auto draggableAttr =
      PyObject_HasAttrString(legend.pyobj().ptr(), "set_draggable") ? "set_draggable" : "draggable";
  legend.pyobj().attr(draggableAttr)(draggable);
  return legend;
}

/**
 * @return The legend instance if exists. None otherwise.
 */
Artist Axes::legendInstance() const {
  GlobalInterpreterLock lock;
  return Artist{pyobj().attr("legend_")};
}

/**
 * @brief Take the data and draw a single Line2D on the axes
 * @param xdata A vector containing the X data
 * @param ydata A vector containing the Y data
 * @param format A format string accepted by matplotlib.axes.Axes.plot.
 * @param label A label for the line
 * @return A new Line2D object that owns the xdata, ydata vectors. If the
 * return value is not captured the line will be automatically removed from
 * the canvas as the vector data will be destroyed.
 */
Line2D Axes::plot(std::vector<double> xdata, std::vector<double> ydata, const QString &format, const QString &label) {
  GlobalInterpreterLock lock;
  auto line2d = plot(std::move(xdata), std::move(ydata), format.toLatin1().constData());
  if (!label.isEmpty()) {
    line2d.pyobj().attr("set_label")(label.toLatin1().constData());
  }
  return line2d;
}

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
Line2D Axes::plot(std::vector<double> xdata, std::vector<double> ydata, const char *format) {
  auto throwIfEmpty = [](const std::vector<double> &data, char vecId) {
    if (data.empty()) {
      throw std::invalid_argument(std::string("Cannot plot line. Empty vector=") + vecId);
    }
  };
  throwIfEmpty(xdata, 'X');
  throwIfEmpty(ydata, 'Y');

  GlobalInterpreterLock lock;
  // Wrap the vector data in a numpy facade to avoid a copy.
  // The vector still owns the data so it needs to be kept alive too
  // It is transferred to the Line2D for this purpose.
  VectorToNDArray<double, WrapReadOnly> wrapNDArray;
  Python::Object xarray{Python::NewRef(wrapNDArray(xdata))}, yarray{Python::NewRef(wrapNDArray(ydata))};

  try {
    return Line2D{pyobj().attr("plot")(xarray, yarray, format)[0], std::move(xdata), std::move(ydata)};
  } catch (Python::ErrorAlreadySet &) {
    throw PythonException();
  }
}

/**
 * Add an arbitrary text label to the canvas
 * @param x X position in data coordinates
 * @param y Y position in data coordinates
 * @param text The string to attach to the canvas
 * @param horizontalAlignment A string indicating the horizontal
 * alignment of the string
 */
Artist Axes::text(double x, double y, const QString &text, const char *horizontalAlignment) {
  GlobalInterpreterLock lock;
  auto args = Python::NewRef(Py_BuildValue("(ffs)", x, y, text.toLatin1().constData()));
  auto kwargs = Python::NewRef(Py_BuildValue("{ss}", "horizontalalignment", horizontalAlignment));
  return Artist(pyobj().attr("text")(*args, **kwargs));
}

/**
 * Add an arbitrary text label to the canvas
 * @param x X position in data coordinates
 * @param y Y position in data coordinates
 * @param text The string to attach to the canvas
 * @param horizontalAlignment A string indicating the horizontal
 * @param transform A transform object to
 * alignment of the string
 */

Artist Axes::text(double x, double y, const QString &text, const char *horizontalAlignment, Transform transform) {
  GlobalInterpreterLock lock;
  auto args = Python::NewRef(Py_BuildValue("(ffs)", x, y, text.toLatin1().constData()));
  auto kwargs = Python::NewRef(
      Py_BuildValue("{sssO}", "horizontalalignment", horizontalAlignment, "transform", transform.pyobj().ptr()));
  return Artist(pyobj().attr("text")(*args, **kwargs));
}

/**
 * @brief Set the X-axis scale to the given value.
 * @param value New scale type. See
 * https://matplotlib.org/api/_as_gen/matplotlib.axes.Axes.set_xscale.html
 * @raises PythonException if the value is unknown
 */
void Axes::setXScale(const char *value) {
  try {
    callMethodNoCheck<void, const char *>(pyobj(), "set_xscale", value);
  } catch (PythonException &) {
    throw std::invalid_argument(std::string("Axes::setXScale - Invalid scale type ") + value);
  }
}

/// @return The scale type of the X axis as a string
QString Axes::getXScale() const {
  GlobalInterpreterLock lock;
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
    callMethodNoCheck<void, const char *>(pyobj(), "set_yscale", value);
  } catch (PythonException &) {
    throw std::invalid_argument(std::string("Axes::setYScale - Invalid scale type ") + value);
  }
}

/// @return The scale type of the Y axis as a string
QString Axes::getYScale() const {
  GlobalInterpreterLock lock;
  return toQString(pyobj().attr("get_yscale")());
}

/**
 * Retrieve the X limits of the axes
 * @return A 2-tuple of (min,max) values for the X axis
 */
std::tuple<double, double> Axes::getXLim() const {
  GlobalInterpreterLock lock;
  return limitsToTuple(pyobj(), "get_xlim");
}

/**
 * Set the limits for the X axis
 * @param min Minimum value
 * @param max Maximum value
 */
void Axes::setXLim(double min, double max) const {
  callMethodNoCheck<void, double, double>(pyobj(), "set_xlim", min, max);
}

/**
 * Retrieve the Y limits of the axes
 * @return A 2-tuple of (min,max) values for the Y axis
 */
std::tuple<double, double> Axes::getYLim() const {
  GlobalInterpreterLock lock;
  return limitsToTuple(pyobj(), "get_ylim");
}

/**
 * Set the limits for the Y axis
 * @param min Minimum value
 * @param max Maximum value
 */
void Axes::setYLim(double min, double max) const {
  callMethodNoCheck<void, double, double>(pyobj(), "set_ylim", min, max);
}

/**
 * @brief Recompute the data limits from the current artists.
 * @param visibleOnly If true then only include visble artists in the
 * calculation
 */
void Axes::relim(bool visibleOnly) { callMethodNoCheck<void, bool>(pyobj(), "relim", visibleOnly); }

/**
 * Calls Axes.autoscale to enable/disable auto scaling
 * @param enable If true enable autoscaling and perform the automatic rescale
 */
void Axes::autoscale(bool enable) { callMethodNoCheck<void, bool>(pyobj(), "autoscale", enable); }

/**
 * Autoscale the view based on the current data limits. Calls
 * Axes.autoscale_view with the tight argument set to None. Autoscaling
 * must be turned on for this to work as expected
 * @param scaleX If true (default) scale the X axis limits
 * @param scaleY If true (default) scale the Y axis limits
 */
void Axes::autoscaleView(bool scaleX, bool scaleY) {
  callMethodNoCheck<void, Python::Object, bool, bool>(pyobj(), "autoscale_view", Python::Object(), scaleX, scaleY);
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
  callMethodNoCheck<void, bool, bool, bool>(pyobj(), "autoscale_view", tight, scaleX, scaleY);
}

/**
 * Returns the blended transform that treats X in data coordinates and Y in axes coordinates
 * https://matplotlib.org/stable/tutorials/advanced/transforms_tutorial.html#blended-transformations
 * @return An instance of a matplotlib.transforms.Transform
 */
Transform Axes::getXAxisTransform() const {
  GlobalInterpreterLock lock;
  return Transform(pyobj().attr("get_xaxis_transform")());
}

} // namespace MantidQt::Widgets::MplCpp
