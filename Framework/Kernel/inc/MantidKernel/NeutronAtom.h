// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_NEUTRONATOM_H_
#define MANTID_KERNEL_NEUTRONATOM_H_

//------------------------------------------------------------------------------
// Include
//------------------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <iosfwd>

namespace Mantid {
namespace PhysicalConstants {

/**
  Structure to store neutronic scattering information for the various elements.
  This is taken from http://www.ncnr.nist.gov/resources/n-lengths/list.html.
*/
struct MANTID_KERNEL_DLL NeutronAtom {

  /// The reference wavelength value for absorption cross sections
  static constexpr double ReferenceLambda{ 1.7982 };

  NeutronAtom(const uint16_t z, const double coh_b_real,
              const double inc_b_real, const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const uint16_t z, const uint16_t a, const double coh_b_real,
              const double inc_b_real, const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const uint16_t z, const uint16_t a, const double coh_b_real,
              const double coh_b_img, const double inc_b_real,
              const double inc_b_img, const double coh_xs, const double inc_xs,
              const double tot_xs, const double abs_xs);

  NeutronAtom(const NeutronAtom &other);

  NeutronAtom &operator=(const NeutronAtom &other);

  NeutronAtom();

  /// The atomic number, or number of protons, for the atom.
  uint16_t z_number;

  /// The total number of protons and neutrons, or mass number,
  /// for the atom for isotopic averages this is set to zero.
  uint16_t a_number;

  /// The real part of the coherent scattering length in fm.
  double coh_scatt_length_real;

  /// The imaginary part of the coherent scattering length in fm.
  double coh_scatt_length_img;

  /// The real part of the incoherent scattering length in fm.
  double inc_scatt_length_real;

  /// The imaginary part of the incoherent scattering length in fm.
  double inc_scatt_length_img;

  /// The coherent scattering cross section in barns.
  double coh_scatt_xs;

  /// The incoherent scattering cross section in barns.
  double inc_scatt_xs;

  /// The total scattering cross section in barns.
  double tot_scatt_xs;

  /// The absorption cross section for 2200m/s neutrons in barns.
  double abs_scatt_xs;

  /// The total scattering length in fm
  double tot_scatt_length;

  /// The coherent scattering length in fm
  double coh_scatt_length;

  /// The incoherent scattering length in fm
  double inc_scatt_length;
};

MANTID_KERNEL_DLL bool operator==(const NeutronAtom &left,
                                  const NeutronAtom &right);
MANTID_KERNEL_DLL bool operator!=(const NeutronAtom &left,
                                  const NeutronAtom &right);

// addition
MANTID_KERNEL_DLL NeutronAtom operator+(const NeutronAtom &left,
                                        const NeutronAtom &right);

// multiplication
MANTID_KERNEL_DLL NeutronAtom operator*(const NeutronAtom &left,
                                        const double right);
MANTID_KERNEL_DLL NeutronAtom operator*(const double left,
                                        const NeutronAtom &right);

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out,
                                           const NeutronAtom &atom);
MANTID_KERNEL_DLL NeutronAtom getNeutronAtom(const uint16_t z_number,
                                             const uint16_t a_number = 0);
MANTID_KERNEL_DLL NeutronAtom getNeutronNoExceptions(const uint16_t z_number,
                                                     const uint16_t a_number);
MANTID_KERNEL_DLL NeutronAtom getNeutronNoExceptions(const NeutronAtom &other);

/// Utility function to calculate scattering lengths from cross-sections.
MANTID_KERNEL_DLL void calculateScatteringLengths(NeutronAtom &atom);

} // Namespace PhysicalConstants
} // Namespace Mantid

#endif /* NEUTRONATOM_H_ */
