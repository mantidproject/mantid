// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    <LI>number density (nAtoms / Angstrom^3)</LI>
  </UL>

  To understand how the effective scattering information is calculated, see
  Sears, Varley F. "Neutron scattering lengths and cross sections." Neutron
  news 3.3 (1992): 26-37. To highlight a point that may be missed, the
  absorption is the only quantity that is wavelength dependent.
*/
class MANTID_KERNEL_DLL Material final {
public:
  /// Structure to hold the information for a parsed chemical formula
  struct FormulaUnit final {
    boost::shared_ptr<PhysicalConstants::Atom> atom;
    double multiplicity;
    FormulaUnit(const boost::shared_ptr<PhysicalConstants::Atom> &atom,
                const double multiplicity);
    FormulaUnit(const PhysicalConstants::Atom &atom, const double multiplicity);
  };

  using ChemicalFormula = std::vector<FormulaUnit>;

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
  /// Get the coherent scattering cross section for a given wavelength in barns.
  double
  cohScatterXSection(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent cross section for a given wavelength in barns.
  double incohScatterXSection(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Return the total scattering cross section for a given wavelength in barns.
  double totalScatterXSection(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the absorption cross section at a given wavelength in barns.
  double
  absorbXSection(const double lambda =
                     PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Returns the linear coefficient of absorption for the material in units of
   * cm^-1
   * this should match the implementation of the iterator version
   */
  double
  linearAbsorpCoef(const double lambda =
                       PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Returns the linear coefficient of absorption for the material in units of
   * cm^-1
   * this should match the implementation of the scalar version
   */
  std::vector<double>
  linearAbsorpCoef(std::vector<double>::const_iterator lambdaBegin,
                   std::vector<double>::const_iterator lambdaEnd) const;

  /// Get the coherent scattering length for a given wavelength in fm
  double
  cohScatterLength(const double lambda =
                       PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double
  incohScatterLength(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Return the total scattering length for a given wavelength in fm
  double
  totalScatterLength(const double lambda =
                         PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the coherent scattering length for a given wavelength in fm
  double cohScatterLengthReal(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the coherent scattering length for a given wavelength in fm
  double cohScatterLengthImg(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double incohScatterLengthReal(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double incohScatterLengthImg(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Get the coherent scattering length squared, \f$<b>^2\f$, for a given
   * wavelength
   * in \f$fm^2\f$.
   */
  double cohScatterLengthSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Get the incoherent length squared, \f$<b>^2\f$, for a given wavelength in
   * \f$fm^2\f$.
   */
  double incohScatterLengthSqrd(
      const double lambda =
          PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Return the total scattering length squared, \f$<b^2>\f$, for a given
   * wavelength
   *  in \f$fm^2\f$.
   */
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
  /// Number density in atoms per A^-3
  double m_numberDensity;
  /// Temperature
  double m_temperature;
  /// Pressure
  double m_pressure;
};

/// Typedef for a shared pointer
using Material_sptr = boost::shared_ptr<Material>;
/// Typedef for a shared pointer to a const object
using Material_const_sptr = boost::shared_ptr<const Material>;
} // namespace Kernel
} // namespace Mantid

#endif // MANTID_GEOMETRY_MATERIAL_H_
