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
  static Python::Object colorsModule{
      Python::NewRef(PyImport_ImportModule("matplotlib.colors"))};
  return colorsModule;
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
          colorsModule().attr("SymLogNorm")(linthresh, linscale, vmin, vmax)) {}

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
