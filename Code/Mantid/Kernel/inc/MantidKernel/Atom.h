#ifndef ATOM_H_
#define ATOM_H_

#include <string>
#include "MantidKernel/DllExport.h"
#include "MantidKernel/NeutronAtom.h"

namespace Mantid
{
namespace PhysicalConstants
{
  /**
   * Struture to hold the common information for an atom. This also allows access
   * to the NeutronAtom information by a pointer. The information in this table
   * is generated using the DANSE project's periodictable python module.
   */
  struct EXPORT_OPT_MANTID_KERNEL Atom {
    Atom(const std::string & symbol, const uint16_t z, const uint16_t a, const double abundance,
         const double mass, const double density);

    /// The atomic symbol. In other words the one or two character abbreviation.
    std::string symbol;

    /// The atomic number, or number of protons, for the atom.
    uint16_t z_number;

    /// The total number of protons and neutrons, or mass number,
    /// for the atom for isotopic averages this is set to zero.
    uint16_t a_number;

    /** The natural abundance of the isotope as a percentage between 0 and 100. For
        isotopic averages this is zero. */
    double abundance;

    /** The atomic mass in units of 'u' (=1g/mol/Na). This is from the normalized
        scale where C12 has an atomic mass of 12. */
    double mass;

    /** The atomic mass density in units of g/cm<SUP>3</SUP>. */
    double mass_density;

    /** The number density in units of cm<SUP>-3</SUP> as calculated from the mass density. */
    double number_density;

    NeutronAtom neutron;
  };

  DLLExport bool operator==(const Atom& left, const Atom & right);
  DLLExport bool operator!=(const Atom& left, const Atom & right);
  DLLExport Atom getAtom(const uint16_t z_number, const uint16_t a_number = 0);
  DLLExport Atom getAtom(const std::string& symbol, const uint16_t a_number = 0);

} // namespace PhysicalConstants
} // namespace Mantid

#endif /* ATOM_H_ */
