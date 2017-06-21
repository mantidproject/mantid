#ifndef MANTID_GEOMETRY_MATERIAL_H_
#define MANTID_GEOMETRY_MATERIAL_H_

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PhysicalConstants.h"
#include <boost/shared_ptr.hpp>
#include <vector>

// Forward Declares
namespace NeXus {
class File;
}

namespace Mantid {

namespace PhysicalConstants {
class Atom;
}

namespace Kernel {

/**
  A material is defined as being composed of a given element, defined as a
  PhysicalConstants::NeutronAtom, with the following properties:

  <UL>
    <LI>temperature (Kelvin)</LI>
    <LI>pressure (KPa) </LI>
    <LI>number density (A^-3)</LI>
  </UL>

Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>

*/
class MANTID_KERNEL_DLL Material {
public:
  /// Structure to hold the information for a parsed chemical formula
  struct FormulaUnit {
    boost::shared_ptr<PhysicalConstants::Atom> atom;
    double multiplicity;
    FormulaUnit(const boost::shared_ptr<PhysicalConstants::Atom> &atom,
                const double multiplicity);
    FormulaUnit(const PhysicalConstants::Atom &atom, const double multiplicity);
  };

  typedef std::vector<FormulaUnit> ChemicalFormula;

  static ChemicalFormula parseChemicalFormula(const std::string chemicalSymbol);

  /// Default constructor. Required for other parts of the code to
  /// function correctly. The material is considered "empty"
  Material();

  /// Construct a material from a known element, with optional
  /// temperature and pressure
  explicit Material(
      const std::string &name, const ChemicalFormula &formula,
      const double numberDensity, const double temperature = 300,
      const double pressure = PhysicalConstants::StandardAtmosphere);
  explicit Material(
      const std::string &name, const PhysicalConstants::NeutronAtom &atom,
      const double numberDensity, const double temperature = 300,
      const double pressure = PhysicalConstants::StandardAtmosphere);
  /// Virtual destructor.
  virtual ~Material() = default;

  /// Returns the name of the material
  const std::string &name() const;
  const Material::ChemicalFormula &chemicalFormula() const;

  /** @name Material properties */
  //@{
  /// Get the number density
  double numberDensity() const;
  /// Get the temperature
  double temperature() const;
  /// Get the pressure
  double pressure() const;
  /// Get the coherent scattering cross section for a given wavelength
  double
  cohScatterXSection(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent cross section for a given wavelength
  double incohScatterXSection(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Return the total scattering cross section for a given wavelength
  double totalScatterXSection(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the absorption cross section at a given wavelength
  double
  absorbXSection(const double lambda =
                     PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the coherent scattering length for a given wavelength
  double
  cohScatterLength(const double lambda =
                       PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength
  double
  incohScatterLength(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Return the total scattering length for a given wavelength
  double
  totalScatterLength(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the coherent scattering length for a given wavelength
  double cohScatterLengthReal(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength
  double incohScatterLengthReal(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the real part of the coherent scattering length <b^2> for a given
  /// wavelength
  double cohScatterLengthRealSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the real part of the incoherent length <b^2> for a given wavelength
  double incohScatterLengthRealSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the coherent scattering length <b^2> for a given wavelength
  double cohScatterLengthSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the incoherent length <b^2> for a given wavelength
  double incohScatterLengthSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Return the total scattering length <b^2> for a given wavelength
  double totalScatterLengthSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  //@}

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  void loadNexus(::NeXus::File *file, const std::string &group);

private:
  /// Update the total atom count
  void countAtoms();

  /// Material name
  std::string m_name;
  /// The normalized chemical formula
  ChemicalFormula m_chemicalFormula;
  /// Total number of atoms
  double m_atomTotal;
  /// Number density in A^-3
  double m_numberDensity;
  /// Temperature
  double m_temperature;
  /// Pressure
  double m_pressure;
};

/// Typedef for a shared pointer
typedef boost::shared_ptr<Material> Material_sptr;
/// Typedef for a shared pointer to a const object
typedef boost::shared_ptr<const Material> Material_const_sptr;
}
}

#endif // MANTID_GEOMETRY_MATERIAL_H_
