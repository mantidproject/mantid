// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/NeutronAtom.h"

#include <memory>
#include <numeric>

namespace Mantid {
using PhysicalConstants::Atom;
using PhysicalConstants::getAtom;
using PhysicalConstants::NeutronAtom;
namespace Kernel {

namespace {
inline bool isEmpty(const std::optional<double> &value) { return !value || value == Mantid::EMPTY_DBL(); }
constexpr auto LARGE_LAMBDA = 100; // Lambda likely to be beyond max lambda in
                                   // any measured spectra. In Angstroms
} // namespace

/**
 * Constructor
 */
MaterialBuilder::MaterialBuilder()
    : m_name(), m_formula(), m_atomicNo(), m_massNo(0), m_numberDensity(), m_packingFraction(), m_zParam(), m_cellVol(),
      m_massDensity(), m_totalXSection(), m_cohXSection(), m_incXSection(), m_absSection(),
      m_numberDensityUnit(NumberDensityUnit::Atoms) {}

/**
 * Set the string name given to the material
 * @param name Human-readable name of the material. Empty string not allowed
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setName(const std::string &name) {
  if (name.empty()) {
    throw std::invalid_argument("MaterialBuilder::setName() - Empty name not allowed.");
  }
  m_name = name;
  return *this;
}

/**
 * Set the chemical formula of the material
 * @param formula Human-readable name of the material
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setFormula(const std::string &formula) {
  if (m_name.empty()) {
    m_name = formula;
  }

  if (m_atomicNo) {
    throw std::runtime_error("MaterialBuilder::setFormula() - Atomic no. "
                             "already set, cannot use formula aswell.");
  }
  if (formula.empty()) {
    throw std::invalid_argument("MaterialBuilder::setFormula() - Empty formula provided.");
  }
  using ChemicalFormula = Material::ChemicalFormula;
  try {
    m_formula = ChemicalFormula(ChemicalFormula(Material::parseChemicalFormula(formula)));
  } catch (std::runtime_error &exc) {
    throw std::invalid_argument("MaterialBuilder::setFormula() - Unable to parse chemical formula: " +
                                std::string(exc.what()));
  }
  return *this;
}

/**
 * Set the type of atom by its atomic number
 * @param atomicNumber Z-number of the atom
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setAtomicNumber(int atomicNumber) {
  if (!m_formula.empty()) {
    throw std::runtime_error("MaterialBuilder::setAtomicNumber() - Formula "
                             "already set, cannot use atomic number aswell.");
  }
  m_atomicNo = atomicNumber;
  return *this;
}

/**
 * Set the isotope by mass number
 * @param massNumber Isotope number of the atom
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setMassNumber(int massNumber) {
  m_massNo = massNumber;
  return *this;
}

/**
 * Set the number density of the sample in atoms or formula units / Angstrom^3
 * @param rho density of the sample in atoms or formula units / Angstrom^3
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setNumberDensity(double rho) {
  if (rho != Mantid::EMPTY_DBL())
    m_numberDensity = rho;
  return *this;
}

/**
 * Set the unit for number density
 * @param unit atoms or formula units / Anstrom^3
 * @return A reference to this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setNumberDensityUnit(NumberDensityUnit unit) {
  m_numberDensityUnit = unit;
  return *this;
}

/**
 * Set the effective number density of the sample in atoms or formula units /
 * Angstrom^3
 * @param rho_eff effective density of the sample in atoms or formula units /
 * Angstrom^3
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setEffectiveNumberDensity(double rho_eff) {
  if (rho_eff != Mantid::EMPTY_DBL())
    m_numberDensityEff = rho_eff;
  return *this;
}

/**
 * Set the packing fraction of the material (default is 1). This is used to
 * infer the effective number density
 */
MaterialBuilder &MaterialBuilder::setPackingFraction(double fraction) {
  if (fraction != Mantid::EMPTY_DBL())
    m_packingFraction = fraction;
  return *this;
}

/**
 * Set the number of formula units in the unit cell
 * @param zparam Number of formula units
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setZParameter(double zparam) {
  m_zParam = zparam;
  return *this;
}

/**
 * Set the volume of unit cell
 * @param cellVolume The volume of the unit cell
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setUnitCellVolume(double cellVolume) {
  m_cellVol = cellVolume;
  return *this;
}

/**
 * Set the mass density of the sample in g / cc
 * @param massDensity The mass density in g / cc
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setMassDensity(double massDensity) {
  m_massDensity = massDensity;
  return *this;
}

/**
 * Set a value for the total scattering cross section
 * @param xsec Value of the cross section
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setTotalScatterXSection(double xsec) {
  if (xsec != Mantid::EMPTY_DBL())
    m_totalXSection = xsec;
  return *this;
}

/**
 * Set a value for the coherent scattering cross section
 * @param xsec Value of the cross section
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setCoherentXSection(double xsec) {
  m_cohXSection = xsec;
  return *this;
}

/**
 * Set a value for the incoherent scattering cross section
 * @param xsec Value of the cross section
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setIncoherentXSection(double xsec) {
  m_incXSection = xsec;
  return *this;
}

/**
 * Set a value for the absorption cross section
 * @param xsec Value of the cross section
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setAbsorptionXSection(double xsec) {
  m_absSection = xsec;
  return *this;
}

/**
 * Set a value for the attenuation profile filename
 * @param filename Name of the file containing the attenuation profile
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setAttenuationProfileFilename(std::string filename) {
  if (!filename.empty()) {
    m_attenuationProfileFileName = filename;
  }
  return *this;
}

/**
 * Set a value for the attenuation profile filename
 * @param filename Name of the file containing the attenuation profile
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setXRayAttenuationProfileFilename(std::string filename) {
  if (!filename.empty()) {
    m_xRayAttenuationProfileFileName = filename;
  }
  return *this;
}

/**
 * Set a value for the attenuation profile search path
 * @param path Path to search
 */
void MaterialBuilder::setAttenuationSearchPath(std::string path) { m_attenuationFileSearchPath = std::move(path); }

/**
 * Build the new Material object from the current set of options
 * @return A new Material object
 */
Material MaterialBuilder::build() const {
  Material::ChemicalFormula formula;

  if (!m_formula.empty()) {
    formula = Material::ChemicalFormula(m_formula);
  } else if (m_atomicNo) {
    formula = createCompositionFromAtomicNumber();
  } else if (!m_totalXSection || !m_cohXSection || !m_incXSection || !m_absSection || !m_numberDensity) {
    throw std::runtime_error("Please specify one of chemical formula or atomic "
                             "number or all cross sections and a number "
                             "density.");
  }

  const auto density_struct = getOrCalculateRhoAndPacking(formula);

  std::unique_ptr<Material> material;
  if (hasOverrideNeutronProperties()) {
    PhysicalConstants::NeutronAtom neutron = generateCustomNeutron();
    material =
        std::make_unique<Material>(m_name, neutron, density_struct.number_density, density_struct.packing_fraction);
  } else {
    material =
        std::make_unique<Material>(m_name, formula, density_struct.number_density, density_struct.packing_fraction);
  }
  if (m_attenuationProfileFileName) {
    AttenuationProfile materialAttenuation(m_attenuationProfileFileName.value(), m_attenuationFileSearchPath,
                                           material.get(), LARGE_LAMBDA);
    material->setAttenuationProfile(materialAttenuation);
  }
  if (m_xRayAttenuationProfileFileName) {
    // don't supply a material so that extrapolation using the neutron tabulated
    // attenuation data is turned off
    AttenuationProfile materialAttenuation(m_xRayAttenuationProfileFileName.value(), m_attenuationFileSearchPath,
                                           nullptr, -1);
    material->setXRayAttenuationProfile(materialAttenuation);
  }
  return *material;
}

/**
 * Create the NeutronAtom object from the atomic number
 * @return A new NeutronAtom object with the defined proprties
 */
Material::ChemicalFormula MaterialBuilder::createCompositionFromAtomicNumber() const {
  Material::FormulaUnit unit{std::make_shared<PhysicalConstants::Atom>(
                                 getAtom(static_cast<uint16_t>(m_atomicNo.value()), static_cast<uint16_t>(m_massNo))),
                             1.};
  Material::ChemicalFormula formula;
  formula.emplace_back(unit);

  return formula;
}

/**
 * Return the manually set density or calculate it from other parameters
 * @param formula The chemical formula to calculate the number density from
 * @return The number density in atoms / Angstrom^3
 */
MaterialBuilder::density_packing
MaterialBuilder::getOrCalculateRhoAndPacking(const Material::ChemicalFormula &formula) const {
  // set packing fraction and both number densities to zero to start
  density_packing result{0., 0., 0.};

  // get the packing fraction
  if (m_packingFraction)
    result.packing_fraction = m_packingFraction.value();

  // if effective density has been specified
  if (m_numberDensityEff)
    result.effective_number_density = m_numberDensityEff.value();

  // total number of atoms is used in both density calculations
  const double totalNumAtoms =
      std::accumulate(formula.cbegin(), formula.cend(), 0.,
                      [](double n, const Material::FormulaUnit &f) { return n + f.multiplicity; });

  // calculate the number density by one of many ways
  if (m_numberDensity) {
    result.number_density = m_numberDensity.value();
    if (m_numberDensityUnit == NumberDensityUnit::FormulaUnits && totalNumAtoms > 0.) {
      result.number_density = m_numberDensity.value() * totalNumAtoms;
    }
  } else if (m_zParam && m_cellVol) {
    result.number_density = totalNumAtoms * m_zParam.value() / m_cellVol.value();
  } else if (!formula.empty() && formula.size() == 1) {
    result.number_density = formula.front().atom->number_density;
  }

  // calculate the effective number density
  if (m_massDensity) {
    // g / cc -> atoms / Angstrom^3
    const double rmm =
        std::accumulate(formula.cbegin(), formula.cend(), 0.,
                        [](double sum, const Material::FormulaUnit &f) { return sum + f.atom->mass * f.multiplicity; });
    result.effective_number_density = (m_massDensity.value() * totalNumAtoms / rmm) * PhysicalConstants::N_A * 1e-24;
  }

  // count the number of values that were set and generate errors
  int count = 0;
  if (result.packing_fraction > 0.)
    count++;
  if (result.effective_number_density > 0.)
    count++;
  if (result.number_density > 0.)
    count++;

  // use this information to set the "missing" of the 3
  if (count == 0) {
    throw std::runtime_error("The number density could not be determined. Please "
                             "provide the number density, ZParameter and unit "
                             "cell volume or mass density.");
  } else if (count == 1) {
    result.packing_fraction = 1.;
    if (result.number_density > 0.)
      result.effective_number_density = result.number_density;
    else if (result.effective_number_density > 0.)
      result.number_density = result.effective_number_density;
    else
      throw std::runtime_error("Must specify the number density in some way");
  } else if (count == 2) {
    if (result.number_density > 0.) {
      if (result.effective_number_density > 0.)
        result.packing_fraction = result.effective_number_density / result.number_density;
      else if (result.packing_fraction > 0.)
        result.effective_number_density = result.packing_fraction * result.number_density;
    } else if (result.effective_number_density > 0.) {
      if (result.number_density > 0.)
        result.packing_fraction = result.effective_number_density / result.number_density;
      else if (result.packing_fraction > 0.)
        result.number_density = result.effective_number_density / result.packing_fraction;
    }
    // do something
  } else if (count == 3) {
    throw std::runtime_error("The number density and effective density were over-determined");
  }

  return result;
}

bool MaterialBuilder::hasOverrideNeutronProperties() const {
  return !isEmpty(m_totalXSection) || !isEmpty(m_cohXSection) || !isEmpty(m_incXSection) || !isEmpty(m_absSection);
}

PhysicalConstants::NeutronAtom MaterialBuilder::generateCustomNeutron() const {
  NeutronAtom neutronAtom(0, 0., 0., 0., 0., 0., 0.);

  // generate the default neutron
  if (m_atomicNo) {
    auto atom = getAtom(static_cast<uint16_t>(m_atomicNo.value()), static_cast<uint16_t>(m_massNo));
    neutronAtom = atom.neutron;
    overrideNeutronProperties(neutronAtom);
  } else if (!m_formula.empty()) {
    double totalNumAtoms = 0.;
    for (const auto &formulaUnit : m_formula) {
      neutronAtom = neutronAtom + formulaUnit.multiplicity * formulaUnit.atom->neutron;
      totalNumAtoms += formulaUnit.multiplicity;
    }
    neutronAtom = (1. / totalNumAtoms) * neutronAtom;
    overrideNeutronProperties(neutronAtom);
  } else {
    neutronAtom.coh_scatt_xs = *m_cohXSection;
    neutronAtom.inc_scatt_xs = *m_incXSection;
    neutronAtom.tot_scatt_xs = *m_totalXSection;
    neutronAtom.abs_scatt_xs = *m_absSection;
    calculateScatteringLengths(neutronAtom);
  }
  neutronAtom.a_number = 0; // signifies custom neutron atom
  neutronAtom.z_number = 0; // signifies custom neutron atom

  return neutronAtom;
}

/**
 * Override default neutron properties with those supplied
 * @param neutron A reference to a NeutronAtom object
 */
void MaterialBuilder::overrideNeutronProperties(PhysicalConstants::NeutronAtom &neutron) const {
  if (!isEmpty(m_totalXSection))
    neutron.tot_scatt_xs = m_totalXSection.value();
  if (!isEmpty(m_cohXSection))
    neutron.coh_scatt_xs = m_cohXSection.value();
  if (!isEmpty(m_incXSection))
    neutron.inc_scatt_xs = m_incXSection.value();
  if (!isEmpty(m_absSection))
    neutron.abs_scatt_xs = m_absSection.value();
}

} // namespace Kernel
} // namespace Mantid
