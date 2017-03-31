#include "MantidKernel/Diffraction.h"
#include <algorithm>

namespace Mantid {
namespace Kernel {
namespace Diffraction {

double calcTofMin(const double difc, const double difa, const double tzero,
                  const double tofmin) {
  if (difa == 0.) {
    if (tzero != 0.) {
      // check for negative d-spacing
      return std::max<double>(tzero, tofmin);
    }
  } else if (difa > 0.) {
    // check for imaginary part in quadratic equation
    return std::max<double>(tzero - .25 * difc * difc / difa, tofmin);
  }

  // everything else is fine so just return supplied tofmin
  return tofmin;
}

/**
 * Returns the maximum TOF that can be used or tofmax. Whichever is smaller. In
 * the case when this is a negative number, just return 0.
 */
double calcTofMax(const double difc, const double difa, const double tzero,
                  const double tofmax) {
  if (difa < 0.) {
    // check for imaginary part in quadratic equation
    if (tzero > 0.) {
      // rather than calling abs multiply difa by -1
      return std::min<double>(tzero + .25 * difc * difc / difa, tofmax);
    } else {
      return 0.;
    }
  }

  // everything else is fine so just return supplied tofmax
  return tofmax;
}

} // namespace Diffraction
} // namespace Kernel
} // namespace Mantid
