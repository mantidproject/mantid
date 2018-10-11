// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidPythonInterface/core/ErrorHandling.h"

using Mantid::PythonInterface::PythonRuntimeError;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
/**
 * @return A reference to the matplotlib.colors module
 */
Python::Object colorsModule() {
  return Python::NewRef(PyImport_ImportModule("matplotlib.colors"));
}

/**
 * @return A reference to the matplotlib.ticker module
 */
Python::Object tickerModule() {
  return Python::NewRef(PyImport_ImportModule("matplotlib.ticker"));
}

/**
 * @return A reference to the matplotlib.ticker module
 */
Python::Object scaleModule() {
  return Python::NewRef(PyImport_ImportModule("matplotlib.scale"));
}

} // namespace

// ------------------------ NormalizeBase---------------------------------------
/**
 * @brief NormalizeBase::NormalizeBase
 * @param obj An existing Normalize instance or subtype
 */
NormalizeBase::NormalizeBase(Python::Object obj)
    : Python::InstanceHolder(std::move(obj), "autoscale") {}

// ------------------------ Normalize ------------------------------------------

/**
 * @brief Construct a Normalize object mapping data from [vmin, vmax]
 * to [0, 1]
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
Normalize::Normalize(double vmin, double vmax)
    : NormalizeBase(colorsModule().attr("Normalize")(vmin, vmax)) {}

// ------------------------ SymLogNorm -----------------------------------------
/// The threshold below which the scale becomes linear
double SymLogNorm::DefaultLinearThreshold = 1e-3;
/// The value to scale the linear range by. Defaults to 1 decade
double SymLogNorm::DefaultLinearScale = 1.0;

/**
 * @brief Construct a SymLogNorm object mapping data from [vmin, vmax]
 * to a symmetric logarithm scale
 * @param linthresh The range within which the plot is linear
 * @param linscale This allows the linear range (-linthresh to linthresh) to be
 * stretched relative to the logarithmic range.
 * See
 * https://matplotlib.org/2.2.3/api/_as_gen/matplotlib.colors.SymLogNorm.html#matplotlib.colors.SymLogNorm
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
SymLogNorm::SymLogNorm(double linthresh, double linscale, double vmin,
                       double vmax)
    : NormalizeBase(
          colorsModule().attr("SymLogNorm")(linthresh, linscale, vmin, vmax)),
      m_linscale(linscale) {}

/**
 * @return An instance of the SymmetricalLogLocator
 */
Python::Object SymLogNorm::tickLocator() const {
  // Create log transform with base=10
  auto transform = scaleModule().attr("SymmetricalLogTransform")(
      10, Python::Object(pyobj().attr("linthresh")), m_linscale);
  return Python::Object(
      tickerModule().attr("SymmetricalLogLocator")(transform));
}

/**
 * @brief SymLogNorm::labelFormatter
 * @return
 */
Python::Object SymLogNorm::labelFormatter() const {
  return Python::Object(tickerModule().attr("LogFormatterMathtext")());
}

// ------------------------ PowerNorm ------------------------------------------

/**
 * @brief Construct a PowerNorm object to map data from [vmin,vmax] to
 * [0,1] pn a power-law scale
 * @param gamma The exponent for the power-law
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
PowerNorm::PowerNorm(double gamma, double vmin, double vmax)
    : NormalizeBase(colorsModule().attr("PowerNorm")(gamma, vmin, vmax)) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
