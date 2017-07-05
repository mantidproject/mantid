//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EQSANSResolution.h"
#include <cmath>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSResolution)

/*
 * Double Boltzmann fit to the TOF resolution as a function of wavelength
 */
double EQSANSResolution::getTOFResolution(double wl) {
  if (wl > 2.0) {
    return 0.0148 * wl * wl * wl - 0.5233 * wl * wl + 6.4797 * wl + 231.99;
  } else {
    return 392.31 * pow(wl, 6) - 3169.3 * pow(wl, 5) + 10445 * pow(wl, 4) -
           17872 * wl * wl * wl + 16509 * wl * wl - 7448.4 * wl + 1280.5;
  }
}

} // namespace Algorithms
} // namespace Mantid
