// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_
#define MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {
namespace Math {
/**
 * Defines a static class for computing the coefficient from a
 * Bose-Einstein distribution for a given energy in meV & temperature in Kelvin
 */
class MANTID_KERNEL_DLL BoseEinsteinDistribution {
public:
  /// Calculate the expected number of particles in an energy state at a given
  /// temperature
  /// for a degenerate distribution with zero chemical potential
  static double n(const double energy, const double temperature);
  /// Calculate the \f$(n+1)\epsilon\f$ for a degenerate distribution with zero
  /// chemical potential
  /// where n is the Bose-Einstein distribution
  static double np1Eps(const double energy, const double temperature);
};
} // namespace Math
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_BOSEEINSTEINDISTRIBUTION_H_ */
