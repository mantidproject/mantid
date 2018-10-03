// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MAGNETICION_H_
#define MAGNETICION_H_

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace PhysicalConstants {
/**
 * Struture to hold information about magnetic form factor for 3d, 4d,
 * rare earth, and actinide atoms and ions. Data is taken from International
 * Tables for Crystalography, volume C, section 4.4.5
 * http://it.iucr.org/Cb/ch4o4v0001/sec4o4o5/
 * and from Kobayashi, Nagao and Ito, Acta. Cryst. A67 473 (2011)
 * http://dx.doi.org/10.1107/S010876731102633X
 * The theory is outlined in the ITC vol C, section 6.1.2
 * http://it.iucr.org/Cb/ch6o1v0001/sec6o1o2/
 */
struct MANTID_KERNEL_DLL MagneticIon {
  /// Default constructor
  MagneticIon();
  /// Construct the Ion with data
  MagneticIon(const char *symbol, const uint16_t charge, const double j0i[8],
              const double j2i[8], const double j4i[8], const double j6i[8],
              const double gi);
  MagneticIon(const char *symbol, const uint16_t charge, const double j0i[9],
              const double j2i[9], const double j4i[9], const double gi);

  /// Returns the value of \<jl(Q)\> for a given Q^2
  double getJLofQsqr(const double qsqr, const uint16_t l) const;
  /// Returns the value of the form factor in the dipole approximation
  double analyticalFormFactor(const double qsqr) const;
  /// Returns the cutoff value for the given form factor approximation
  static double formFactorCutOff();

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

  // The Lande g-factor of this ion
  double g;
};

/// Returns the magnetic ion for the given symbol and charge
MANTID_KERNEL_DLL const MagneticIon &getMagneticIon(const std::string &symbol,
                                                    const uint16_t charge);
/// Returns the magnetic ion from a combined symbol and charge given as string
MANTID_KERNEL_DLL const MagneticIon &getMagneticIon(const std::string &symbol);
/// Returns the Lth-coefficients for the given ion
MANTID_KERNEL_DLL std::vector<double>
getJL(const std::string &symbol, const uint16_t charge, const uint16_t l = 0);

/// Returns a list of all ions
MANTID_KERNEL_DLL std::vector<std::string> getMagneticIonList();

} // namespace PhysicalConstants
} // namespace Mantid

#endif /* MAGNETICION_H_ */
