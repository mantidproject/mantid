#include "MantidKernel/Diffraction.h"
#include <algorithm>
#include <cmath>

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

// ----------------------------------------------------------------------------
// convert from d-spacing to time-of-flight
namespace { // anonymous namespace
/// Applies the equation d=(TOF-tzero)/difc
struct tof_to_d_difc_only {
  explicit tof_to_d_difc_only(const double difc) : factor(1. / difc) {}

  double operator()(const double tof) const { return factor * tof; }

  /// 1./difc
  double factor;
};

/// Applies the equation d=(TOF-tzero)/difc
struct tof_to_d_difc_and_tzero {
  tof_to_d_difc_and_tzero(const double difc, const double tzero)
      : factor(1. / difc), offset(-1. * tzero / difc) {}

  double operator()(const double tof) const { return factor * tof + offset; }

  /// 1./difc
  double factor;
  /// -tzero/difc
  double offset;
};

struct tof_to_d {
  tof_to_d(const double difc, const double difa, const double tzero) {
    factor1 = -0.5 * difc / difa;
    factor2 = 1. / difa;
    factor3 = (factor1 * factor1) - (tzero / difa);
  }

  double operator()(const double tof) const {
    double second = std::sqrt((tof * factor2) + factor3);
    if (second < factor1)
      return factor1 - second;
    else {
      return factor1 + second;
    }
  }

  /// -0.5*difc/difa
  double factor1;
  /// 1/difa
  double factor2;
  /// (0.5*difc/difa)^2 - (tzero/difa)
  double factor3;
};
} // anonymous namespace

std::function<double(double)> getTofToDConversionFunc(const double difc,
                                                      const double difa,
                                                      const double tzero) {
  if (difa == 0.) {
    if (tzero == 0.) {
      return tof_to_d_difc_only(difc);
    } else {
      return tof_to_d_difc_and_tzero(difc, tzero);
    }
  } else { // difa != 0.
    return tof_to_d(difc, difa, tzero);
  }
}

// ----------------------------------------------------------------------------
// convert from d-spacing to time-of-flight
namespace { // anonymous namespace
struct d_to_tof_difc_and_tzero {
  d_to_tof_difc_and_tzero(const double difc, const double tzero) {
    this->difc = difc;
    this->tzero = tzero;
  }

  double operator()(const double dspacing) const {
    return difc * dspacing + tzero;
  }

  double difc;
  double tzero;
};

struct d_to_tof {
  d_to_tof(const double difc, const double difa, const double tzero) {
    this->difc = difc;
    this->difa = difa;
    this->tzero = tzero;
  }

  double operator()(const double dspacing) const {
    return difc * dspacing + difa * dspacing * dspacing + tzero;
  }

  double difc;
  double difa;
  double tzero;
};
} // anonymous namespace

std::function<double(double)> getDToTofConversionFunc(const double difc,
                                                      const double difa,
                                                      const double tzero) {
  if (difa == 0.) {
    return d_to_tof_difc_and_tzero(difc, tzero);
  } else {
    return d_to_tof(difc, difa, tzero);
  }
}

} // namespace Diffraction
} // namespace Kernel
} // namespace Mantid
