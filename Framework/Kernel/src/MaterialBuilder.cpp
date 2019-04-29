// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/make_unique.h"
#include <boost/make_shared.hpp>
#include <numeric>

namespace Mantid {
using PhysicalConstants::Atom;
using PhysicalConstants::NeutronAtom;
using PhysicalConstants::getAtom;
namespace Kernel {

namespace {
inline bool isEmpty(const boost::optional<double> value) {
  return !value || value == Mantid::EMPTY_DBL();
}
} // namespace

/**
 * Constructor
 */
MaterialBuilder::MaterialBuilder()
    : m_name(), m_formula(), m_atomicNo(), m_massNo(0), m_numberDensity(),
      m_zParam(), m_cellVol(), m_massDensity(), m_totalXSection(),
      m_cohXSection(), m_incXSection(), m_absSection(),
      m_numberDensityUnit(NumberDensityUnit::Atoms) {}

/**
 * Set the string name given to the material
 * @param name Human-readable name of the material. Empty string not allowed
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setName(const std::string &name) {
  if (name.empty()) {
    throw std::invalid_argument(
        "MaterialBuilder::setName() - Empty name not allowed.");
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
    throw std::invalid_argument(
        "MaterialBuilder::setFormula() - Empty formula provided.");
  }
  using ChemicalFormula = Material::ChemicalFormula;
  try {
    m_formula = ChemicalFormula(
        ChemicalFormula(Material::parseChemicalFormula(formula)));
  } catch (std::runtime_error &exc) {
    throw std::invalid_argument(
        "MaterialBuilder::setFormula() - Unable to parse chemical formula: " +
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
 * Set the number of formula units in the unit cell
 * @param zparam Number of formula units
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setZParameter(double zparam) {
  if (m_massDensity) {
    throw std::runtime_error("MaterialBuilder::setZParameter() - Mass density "
                             "already set, cannot use Z parameter as well.");
  }
  m_zParam = zparam;
  return *this;
}

/**
 * Set the volume of unit cell
 * @param cellVolume The volume of the unit cell
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setUnitCellVolume(double cellVolume) {
  if (m_massDensity) {
    throw std::runtime_error(
        "MaterialBuilder::setUnitCellVolume() - Mass density "
        "already set, cannot use unit cell volume as well.");
  }
  m_cellVol = cellVolume;
  return *this;
}

/**
 * Set the mass density of the sample in g / cc
 * @param massDensity The mass density in g / cc
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setMassDensity(double massDensity) {
  if (m_zParam) {
    throw std::runtime_error("MaterialBuilder::setMassDensity() - Z parameter "
                             "already set, cannot use mass density as well.");
  }
  if (m_cellVol) {
    throw std::runtime_error(
        "MaterialBuilder::setMassDensity() - Unit cell "
        "volume already set, cannot use mass density as well.");
  }
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
 * Build the new Material object from the current set of options
 * @return A new Material object
 */
Material MaterialBuilder::build() const {
  Material::ChemicalFormula formula;

  if (!m_formula.empty()) {
    formula = Material::ChemicalFormula(m_formula);
  } else if (m_atomicNo) {
    formula = createCompositionFromAtomicNumber();
  } else if (!m_totalXSection || !m_cohXSection || !m_incXSection ||
             !m_absSection || !m_numberDensity) {
    throw std::runtime_error("Please specify one of chemical formula or atomic "
                             "number or all cross sections and a number "
                             "density.");
  }

  const double density = getOrCalculateRho(formula);
  if (hasOverrideNeutronProperties()) {
    PhysicalConstants::NeutronAtom neutron = generateCustomNeutron();
    return Material(m_name, neutron, density);
  } else {
    return Material(m_name, formula, density);
  }
}

/**
 * Create the NeutronAtom object from the atomic number
 * @return A new NeutronAtom object with the defined proprties
 */
Material::ChemicalFormula
MaterialBuilder::createCompositionFromAtomicNumber() const {
  Material::FormulaUnit unit{
      boost::make_shared<PhysicalConstants::Atom>(
          getAtom(static_cast<uint16_t>(m_atomicNo.get()),
                  static_cast<uint16_t>(m_massNo))),
      1.};
  Material::ChemicalFormula formula;
  formula.push_back(unit);

  return formula;
}

/**
 * Return the manually set density or calculate it from other parameters
 * @param formula The chemical formula to calculate the number density from
 * @return The number density in atoms / Angstrom^3
 */
double MaterialBuilder::getOrCalculateRho(
    const Material::ChemicalFormula &formula) const {
  double density;
  if (m_numberDensity && m_numberDensityUnit == NumberDensityUnit::Atoms) {
    density = m_numberDensity.get();
  } else {
    const double totalNumAtoms =
        std::accumulate(formula.cbegin(), formula.cend(), 0.,
                        [](double n, const Material::FormulaUnit &f) {
                          return n + f.multiplicity;
                        });
    if (m_numberDensity &&
        m_numberDensityUnit == NumberDensityUnit::FormulaUnits) {
      density = m_numberDensity.get() * totalNumAtoms;
    } else if (m_zParam && m_cellVol) {
      density = totalNumAtoms * m_zParam.get() / m_cellVol.get();
    } else if (m_massDensity) {
      // g / cc -> atoms / Angstrom^3
      const double rmm =
          std::accumulate(formula.cbegin(), formula.cend(), 0.,
                          [](double sum, const Material::FormulaUnit &f) {
                            return sum + f.atom->mass * f.multiplicity;
                          });
      density = (m_massDensity.get() * totalNumAtoms / rmm) *
                PhysicalConstants::N_A * 1e-24;
    } else if (!m_formula.empty() && m_formula.size() == 1) {
      density = m_formula.front().atom->number_density;
    } else {
      throw std::runtime_error(
          "The number density could not be determined. Please "
          "provide the number density, ZParameter and unit "
          "cell volume or mass density.");
    }
  }
  return density;
}

bool MaterialBuilder::hasOverrideNeutronProperties() const {
  return !isEmpty(m_totalXSection) || !isEmpty(m_cohXSection) ||
         !isEmpty(m_incXSection) || !isEmpty(m_absSection);
}

PhysicalConstants::NeutronAtom MaterialBuilder::generateCustomNeutron() const {
  NeutronAtom neutronAtom(0, 0., 0., 0., 0., 0., 0.);

  // generate the default neutron
  if (m_atomicNo) {
    auto atom = getAtom(static_cast<uint16_t>(m_atomicNo.get()),
                        static_cast<uint16_t>(m_massNo));
    neutronAtom = atom.neutron;
    overrideNeutronProperties(neutronAtom);
  } else if (!m_formula.empty()) {
    double totalNumAtoms = 0.;
    for (const auto &formulaUnit : m_formula) {
      neutronAtom =
          neutronAtom + formulaUnit.multiplicity * formulaUnit.atom->neutron;
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
void MaterialBuilder::overrideNeutronProperties(
    PhysicalConstants::NeutronAtom &neutron) const {
  if (!isEmpty(m_totalXSection))
    neutron.tot_scatt_xs = m_totalXSection.get();
  if (!isEmpty(m_cohXSection))
    neutron.coh_scatt_xs = m_cohXSection.get();
  if (!isEmpty(m_incXSection))
    neutron.inc_scatt_xs = m_incXSection.get();
  if (!isEmpty(m_absSection))
    neutron.abs_scatt_xs = m_absSection.get();
}

} // namespace Kernel
} // namespace Mantid
