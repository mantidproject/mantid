// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PhysicalConstants.h"
#include <memory>
#include <optional>
#include <string>
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

class AttenuationProfile;

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
    std::shared_ptr<PhysicalConstants::Atom> atom;
    double multiplicity;
    FormulaUnit(std::shared_ptr<PhysicalConstants::Atom> atom, const double multiplicity);
    FormulaUnit(const PhysicalConstants::Atom &atom, const double multiplicity);
  };

  using ChemicalFormula = std::vector<FormulaUnit>;

  static ChemicalFormula parseChemicalFormula(const std::string &chemicalSymbol);

  /// Default constructor. Required for other parts of the code to
  /// function correctly. The material is considered "empty"
  Material();

  /// Construct a material from a known element, with optional
  /// temperature and pressure
  explicit Material(std::string name, const ChemicalFormula &formula, const double numberDensity,
                    const double packingFraction = 1, const double temperature = 300,
                    const double pressure = PhysicalConstants::StandardAtmosphere);
  explicit Material(std::string name, const PhysicalConstants::NeutronAtom &atom, const double numberDensity,
                    const double packingFraction = 1, const double temperature = 300,
                    const double pressure = PhysicalConstants::StandardAtmosphere);
  /// Virtual destructor.
  virtual ~Material() = default;

  /// Allow an explicit attenuation profile to be loaded onto the material
  /// that overrides the standard linear absorption coefficient
  void setAttenuationProfile(AttenuationProfile attenuationOverride);
  void setXRayAttenuationProfile(AttenuationProfile attenuationProfile);

  /// Returns the name of the material
  const std::string &name() const;
  const Material::ChemicalFormula &chemicalFormula() const;

  /** @name Material properties */
  //@{
  /// Get the number density
  double numberDensity() const;
  /// Get the effective number density
  double numberDensityEffective() const;
  /// Get the packing fraction
  double packingFraction() const;
  /// The total number of atoms in the formula
  double totalAtoms() const;
  /// Get the temperature
  double temperature() const;
  /// Get the pressure
  double pressure() const;
  /// Get the coherent scattering cross section for a given wavelength in barns.
  double cohScatterXSection() const;
  /// Get the incoherent cross section for a given wavelength in barns.
  double incohScatterXSection() const;
  /// Return the total scattering cross section for a given wavelength in barns.
  double totalScatterXSection() const;
  /// Get the absorption cross section at a given wavelength in barns.
  double absorbXSection(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  double attenuationCoefficient(const double lambda) const;
  /// Compute the attenuation at a given wavelength over the given distance
  double attenuation(const double distance,
                     const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Compute the x-ray attenuation at a given energy over the given distance
  double xRayAttenuation(const double distance, const double energy) const;

  /**
   * Returns the linear coefficient of absorption for the material in units of
   * cm^-1
   * this should match the implementation of the iterator version
   */
  double linearAbsorpCoef(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Returns the linear coefficient of absorption for the material in units of
   * cm^-1
   * this should match the implementation of the scalar version
   */
  std::vector<double> linearAbsorpCoef(std::vector<double>::const_iterator lambdaBegin,
                                       std::vector<double>::const_iterator lambdaEnd) const;

  /// Get the coherent scattering length for a given wavelength in fm
  double cohScatterLength(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double incohScatterLength(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Return the total scattering length for a given wavelength in fm
  double totalScatterLength(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /// Get the coherent scattering length for a given wavelength in fm
  double cohScatterLengthReal(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the coherent scattering length for a given wavelength in fm
  double cohScatterLengthImg(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double incohScatterLengthReal(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  /// Get the incoherent length for a given wavelength in fm
  double incohScatterLengthImg(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Get the coherent scattering length squared, \f$<b>^2\f$, for a given
   * wavelength
   * in \f$fm^2\f$.
   */
  double cohScatterLengthSqrd(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Get the incoherent length squared, \f$<b>^2\f$, for a given wavelength in
   * \f$fm^2\f$.
   */
  double incohScatterLengthSqrd(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;

  /**
   * Return the total scattering length squared, \f$<b^2>\f$, for a given
   * wavelength
   *  in \f$fm^2\f$.
   */
  double totalScatterLengthSqrd(const double lambda = PhysicalConstants::NeutronAtom::ReferenceLambda) const;
  //@}

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  void loadNexus(::NeXus::File *file, const std::string &group);

  bool hasValidXRayAttenuationProfile();

private:
  /// Update the total atom count
  void countAtoms();
  /// Update the linear absorption x section (by wavelength)
  void calculateLinearAbsorpXSectionByWL();
  /// Update the total scatter x section
  void calculateTotalScatterXSection();

  /// Material name
  std::string m_name;
  /// The normalized chemical formula
  ChemicalFormula m_chemicalFormula;
  /// Total number of atoms
  double m_atomTotal;
  /// Number density in atoms per A^-3
  double m_numberDensity;
  /// Packing fraction should be between 0 and 2
  double m_packingFraction;
  /// Temperature
  double m_temperature;
  /// Pressure
  double m_pressure;
  double m_linearAbsorpXSectionByWL;
  double m_totalScatterXSection;

  std::optional<AttenuationProfile> m_attenuationOverride;
  std::optional<AttenuationProfile> m_xRayAttenuationProfile;
};

/// Typedef for a shared pointer
using Material_sptr = std::shared_ptr<Material>;
/// Typedef for a shared pointer to a const object
using Material_const_sptr = std::shared_ptr<const Material>;
} // namespace Kernel
} // namespace Mantid
