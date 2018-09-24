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
  static auto colorsModule{
      Python::NewRef(PyImport_ImportModule("matplotlib.colors"))};
  return colorsModule;
}
} // namespace

// ------------------------ Normalize ------------------------------------------

/**
 * @brief Construct a Normalize object mapping data from [vmin, vmax]
 * to [0, 1]
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
Normalize::Normalize(double vmin, double vmax)
    : Python::InstanceHolder(colorsModule().attr("Normalize")(vmin, vmax),
                             "autoscale") {}

// ------------------------ SymLogNorm -----------------------------------------

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
    : Python::InstanceHolder(
          colorsModule().attr("SymLogNorm")(linthresh, linscale, vmin, vmax),
          "autoscale") {}

// ------------------------ PowerNorm ------------------------------------------

/**
 * @brief Construct a PowerNorm object to map data from [vmin,vmax] to
 * [0,1] pn a power-law scale
 * @param gamma The exponent for the power-law
 * @param vmin Minimum value of the data interval
 * @param vmax Maximum value of the data interval
 */
PowerNorm::PowerNorm(double gamma, double vmin, double vmax) :
    Python::InstanceHolder(colorsModule().attr("PowerNorm")(gamma, vmin, vmax),
                           "autoscale") {}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
