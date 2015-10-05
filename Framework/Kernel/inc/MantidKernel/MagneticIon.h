#ifndef MAGNETICION_H_
#define MAGNETICION_H_

#include <string>
#include <vector>
#include <map>
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/PhysicalConstants.h"

namespace Mantid {
namespace PhysicalConstants {
/**
 * Struture to hold information about magnetic form factor for 3d, 4d,
 * rare earth, and actinide atoms and ions. Data is taken from International
 * Tables of
 * Crystalography, volume C, section 4.4.5
 * http://it.iucr.org/Cb/ch4o4v0001/sec4o4o5/
 */
struct MANTID_KERNEL_DLL MagneticIon {
  /// Default constructor
  MagneticIon();
  /// Construct the Ion with data
  MagneticIon(const std::string symbol, const uint16_t charge,
              const double j0[8], const double j2[8], const double j4[8],
              const double j6[8]);

  /// Returns the value of the form factor for the given J/L
  double analyticalFormFactor(const double qsqr, const uint16_t j,
                              const uint16_t l = 0) const;
  /// Returns the cutoff value for the given form factor approximation
  static double formFactorCutOff(const uint16_t j, const uint16_t l);

  /// The atomic symbol. In other words the one or two character abbreviation.
  std::string symbol;

  /// The charge of the ion, or 0 for neutral atom. Note thet all charges are
  /// not negative
  uint16_t charge;

  /// A vector containing A, a, B, b, C, c D, e for each \<j0\>
  std::vector<double> j0;
  /// A vector containing A, a, B, b, C, c D, e for each \<j2\>
  std::vector<double> j2;
  /// A vector containing A, a, B, b, C, c D, e for each \<j4\>
  std::vector<double> j4;
  /// A vector containing A, a, B, b, C, c D, e for each \<j6\>
  std::vector<double> j6;
};

/// Returns the magnetic ion for the given symbol and charge
MANTID_KERNEL_DLL const MagneticIon &getMagneticIon(const std::string &symbol,
                                                    const uint16_t charge);
/// Returns the magnetic ion from a combined symbol and charge given as string
MANTID_KERNEL_DLL const MagneticIon &getMagneticIon(const std::string &symbol);
/// Returns the Lth-coefficients for the given ion
MANTID_KERNEL_DLL std::vector<double>
getJL(const std::string &symbol, const uint16_t charge, const uint16_t l = 0);

} // namespace PhysicalConstants
} // namespace Mantid

#endif /* MAGNETICION_H_ */
