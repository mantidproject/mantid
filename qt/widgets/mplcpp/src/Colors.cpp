// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

#include <optional>

#include <algorithm>
#include <numeric>
#include <tuple>

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;
using std::nullopt;
using std::optional;

using OptionalTupleDouble = optional<std::tuple<double, double>>;

using namespace Mantid::PythonInterface;
using namespace MantidQt::Widgets::Common;

namespace MantidQt::Widgets::MplCpp {

namespace {
/**
 * @return A reference to the matplotlib.colors module
 */
Python::Object colorsModule() { return Python::NewRef(PyImport_ImportModule("matplotlib.colors")); }

// Factory function for creating a Normalize instance
// Holds the GIL
Python::Object createNormalize(OptionalTupleDouble clim = std::nullopt) {
  GlobalInterpreterLock lock;
  if (clim.has_value()) {
    const auto &range = clim.value();
    return colorsModule().attr("Normalize")(std::get<0>(range), std::get<1>(range));
  } else
    return colorsModule().attr("Normalize")();
}

// Factory function for creating a SymLogNorm instance
// Holds the GIL
Python::Object createSymLog(double linthresh, double linscale, OptionalTupleDouble clim = std::nullopt) {
  GlobalInterpreterLock lock;
  if (clim.has_value()) {
    const auto &range = clim.value();
    return colorsModule().attr("SymLogNorm")(linthresh, linscale, std::get<0>(range), std::get<1>(range));
  } else
    return colorsModule().attr("SymLogNorm")(linthresh, linscale);
}

// Factory function for creating a SymLogNorm instance
// Holds the GIL
Python::Object createPowerNorm(double gamma, OptionalTupleDouble clim = std::nullopt) {
  GlobalInterpreterLock lock;
  if (clim.has_value()) {
    const auto &range = clim.value();
    return colorsModule().attr("PowerNorm")(gamma, std::get<0>(range), std::get<1>(range));
  } else {
    return colorsModule().attr("PowerNorm")(gamma);
  }
}

/**
 * @return A reference to the matplotlib.ticker module
 */
Python::Object tickerModule() {
  GlobalInterpreterLock lock;
  return Python::NewRef(PyImport_ImportModule("matplotlib.ticker"));
}

/**
 * @return A reference to the matplotlib.ticker module
 */
Python::Object scaleModule() {
  GlobalInterpreterLock lock;
  return Python::NewRef(PyImport_ImportModule("matplotlib.scale"));
}

} // namespace

// ------------------------ NormalizeBase---------------------------------------
/**
 * Calls autoscale([vmin,vmax]) on the normalize instance. This
 * forces any invalid values to a valid range
 * @param clim A 2-tuple of the scale range
 * @return A 2-tuple of the new scale values
 */
std::tuple<double, double> NormalizeBase::autoscale(std::tuple<double, double> clim) {
  GlobalInterpreterLock lock;
  pyobj().attr("autoscale")(Python::NewRef(Py_BuildValue("(ff)", std::get<0>(clim), std::get<1>(clim))));
  Python::Object scaleMin(pyobj().attr("vmin")), scaleMax(pyobj().attr("vmax"));
  return std::make_tuple(PyFloat_AsDouble(scaleMin.ptr()), PyFloat_AsDouble(scaleMax.ptr()));
}

/**
 * Constructor
 * @param obj An existing Normalize instance or subtype
 */
NormalizeBase::NormalizeBase(Python::Object obj) : Python::InstanceHolder(std::move(obj)) {}

// ------------------------ Normalize ------------------------------------------

/**
 * @brief Construct a Normalize object mapping data from [vmin, vmax]
 * to [0, 1] leaving the vmin,vmax limits unset. A call to autoscale
 * will be required
 */
Normalize::Normalize() : NormalizeBase(createNormalize()) {}

/**
 * @brief Construct a Normalize object mapping data from [vmin, vmax]
 * to [0, 1]
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval */
Normalize::Normalize(double vmin, double vmax) : NormalizeBase(createNormalize(std::make_tuple(vmin, vmax))) {}

// ------------------------ SymLogNorm -----------------------------------------
/// The threshold below which the scale becomes linear
double SymLogNorm::DefaultLinearThreshold = 1e-3;
/// The value to scale the linear range by. Defaults to 1 decade
double SymLogNorm::DefaultLinearScale = 1.0;

/**
 * @brief Construct a SymLogNorm object mapping data from [vmin, vmax]
 * to a symmetric logarithm scale. Default limits are None so autoscale
 * will need to be called
 * @param linthresh The range within which the plot is linear
 * @param linscale This allows the linear range (-linthresh to linthresh) to be
 * stretched relative to the logarithmic range.
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.SymLogNorm.html#matplotlib.colors.SymLogNorm
 */
SymLogNorm::SymLogNorm(double linthresh, double linscale)
    : NormalizeBase(createSymLog(linthresh, linscale)), m_linscale(linscale) {}

/**
 * @brief Construct a SymLogNorm object mapping data from [vmin, vmax]
 * to a symmetric logarithm scale. Default limits are None so autoscale
 * will need to be called
 * @param linthresh The range within which the plot is linear
 * @param linscale This allows the linear range (-linthresh to linthresh) to be
 * stretched relative to the logarithmic range.
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.SymLogNorm.html#matplotlib.colors.SymLogNorm
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
SymLogNorm::SymLogNorm(double linthresh, double linscale, double vmin, double vmax)
    : NormalizeBase(createSymLog(linthresh, linscale, std::make_tuple(vmin, vmax))), m_linscale(linscale) {}

/**
 * @return An instance of the SymmetricalLogLocator
 */
Python::Object SymLogNorm::tickLocator() const {
  GlobalInterpreterLock lock;
  // Create log transform with base=10
  auto transform =
      scaleModule().attr("SymmetricalLogTransform")(10, Python::Object(pyobj().attr("linthresh")), m_linscale);

  // Sets the subs parameter to be [1,2,...,10]. The parameter determines where
  // the ticks on the colorbar are placed and setting it to this ensures that
  // any range of values will have ticks.
  std::vector<float> subsVector(10);
  std::iota(subsVector.begin(), subsVector.end(), 1.f);
  auto subs = Converters::ToPyList<float>()(subsVector);

  return Python::Object(tickerModule().attr("SymmetricalLogLocator")(transform, subs));
}

/**
 * @brief SymLogNorm::labelFormatter
 * @return
 */
Python::Object SymLogNorm::labelFormatter() const {
  GlobalInterpreterLock lock;
  return Python::Object(tickerModule().attr("LogFormatterSciNotation")());
}

// ------------------------ PowerNorm ------------------------------------------
/**
 * @brief Construct a PowerNorm object to map data from [vmin,vmax] to
 * [0,1] pn a power-law scale. Default limits are None so autoscale
 * will need to be called
 * @param gamma The exponent for the power-law
 */
PowerNorm::PowerNorm(double gamma) : NormalizeBase(createPowerNorm(gamma)) {}

/**
 * @brief Construct a PowerNorm object to map data from [vmin,vmax] to
 * [0,1] pn a power-law scale
 * @param gamma The exponent for the power-law
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
PowerNorm::PowerNorm(double gamma, double vmin, double vmax)
    : NormalizeBase(createPowerNorm(gamma, std::make_tuple(vmin, vmax))) {}

std::tuple<double, double> PowerNorm::autoscale(std::tuple<double, double> clim) {
  // Clipping was removed in matplotlib v3.2.0rc2 (Upstream PR #10234) so
  // autoscale will now map [-x,y] -> [0,1] which causes weird color bar scales
  // We now manually clamp min value to 0 so we map from [0,n]->[0,1]
  std::get<0>(clim) = std::max(0., std::get<0>(clim));
  return NormalizeBase::autoscale(clim);
}

} // namespace MantidQt::Widgets::MplCpp
