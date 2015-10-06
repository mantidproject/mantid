//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EQSANSResolution.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/VectorHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSResolution)

/*
 * Double Boltzmann fit to the TOF resolution as a function of wavelength
 */
double EQSANSResolution::getTOFResolution(double wl) {
  double y0 = -388;
  double A = 3838;
  double frac = 0.04398;
  double x01 = 3.392;
  double x02 = 134.3;
  double k1 = -0.5587;
  double k2 = -65.46;

  return y0 +
         A * (frac / (1 + std::exp((wl - x01) / k1)) +
              (1 - frac) / (1 + std::exp((wl - x02) / k2)));
}

} // namespace Algorithms
} // namespace Mantid
