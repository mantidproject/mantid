// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/NeutronAtom.h"
#include <string>

namespace Mantid {
namespace PhysicalConstants {
/**
 * Struture to hold the common information for an atom. This also allows access
 * to the NeutronAtom information by a pointer. The information in this table
 * is generated using the DANSE project's periodictable python module.
 */
class MANTID_KERNEL_DLL Atom {
public:
  /// Standard constructor
  Atom(std::string symbol, const uint16_t z, const uint16_t a, const double abundance, const double mass,
       const double density);
  /// Copy constructor
  Atom(const Atom &other);
  /// Create user defined atom from neutronic information
  Atom(const NeutronAtom &neutron);

  /// The atomic symbol. In other words the one or two character abbreviation.
  const std::string symbol;

  /// The atomic number, or number of protons, for the atom.
  const uint16_t z_number;

  /// The total number of protons and neutrons, or mass number,
  /// for the atom for isotopic averages this is set to zero.
  const uint16_t a_number;

  /** The natural abundance of the isotope as a percentage between 0 and 100.
     For
      isotopic averages this is zero. */
  const double abundance;

  /** The atomic mass in units of 'u' (=1g/mol/Na). This is from the normalized
      scale where C12 has an atomic mass of 12. */
  const double mass;

  /** The atomic mass density in units of g/cm<SUP>3</SUP>. */
  const double mass_density;

  /** The number density in units of cm<SUP>-3</SUP> as calculated from the mass
   * density. */
  const double number_density;

  /// Handle to class containing neutronic atomic properties.
  const NeutronAtom neutron;
};

/// Equality operator overload
MANTID_KERNEL_DLL bool operator==(const Atom &left, const Atom &right);
/// Inequality operator overload
MANTID_KERNEL_DLL bool operator!=(const Atom &left, const Atom &right);
/// Stream operator overload
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out, const Atom &atom);
MANTID_KERNEL_DLL const Atom &getAtom(const uint16_t z_number, const uint16_t a_number = 0);
MANTID_KERNEL_DLL const Atom &getAtom(const std::string &symbol, const uint16_t a_number = 0);

} // namespace PhysicalConstants
} // namespace Mantid
