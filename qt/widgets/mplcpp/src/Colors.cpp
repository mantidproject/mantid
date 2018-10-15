// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"

using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::PythonException;

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

// Factory function for creating a Normalize instance
// Holds the GIL
Python::Object createNormalize(double vmin, double vmax) {
  GlobalInterpreterLock lock;
  return colorsModule().attr("Normalize")(vmin, vmax);
}

// Factory function for creating a SymLogNorm instance
// Holds the GIL
Python::Object createSymLog(double linthresh, double linscale, double vmin,
                            double vmax) {
  GlobalInterpreterLock lock;
  return colorsModule().attr("SymLogNorm")(linthresh, linscale, vmin, vmax);
}

// Factory function for creating a SymLogNorm instance
// Holds the GIL
Python::Object createPowerNorm(double gamma, double vmin, double vmax) {
  GlobalInterpreterLock lock;
  return colorsModule().attr("PowerNorm")(gamma, vmin, vmax);
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
    : NormalizeBase(createNormalize(vmin, vmax)) {}

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
    : NormalizeBase(createSymLog(linthresh, linscale, vmin, vmax)),
      m_linscale(linscale) {}

/**
 * @return An instance of the SymmetricalLogLocator
 */
Python::Object SymLogNorm::tickLocator() const {
  GlobalInterpreterLock lock;
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
  GlobalInterpreterLock lock;
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
    : NormalizeBase(createPowerNorm(gamma, vmin, vmax)) {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
