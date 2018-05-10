#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/make_unique.h"
#include <boost/make_shared.hpp>

namespace Mantid {
using PhysicalConstants::Atom;
using PhysicalConstants::NeutronAtom;
using PhysicalConstants::getAtom;
namespace Kernel {

namespace {
inline bool isEmpty(const boost::optional<double> value) {
  if (!value)
    return true;
  return (value == Mantid::EMPTY_DBL());
}
} // namespace

/**
 * Constructor
 */
MaterialBuilder::MaterialBuilder()
    : m_name(), m_formula(), m_atomicNo(), m_massNo(0), m_numberDensity(),
      m_zParam(), m_cellVol(), m_massDensity(), m_totalXSection(),
      m_cohXSection(), m_incXSection(), m_absSection() {}

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
    m_formula = Mantid::Kernel::make_unique<ChemicalFormula>(
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
  if (m_formula) {
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
 * Set the number density of the sample in atoms / Angstrom^3
 * @param rho density of the sample in atoms / Angstrom^3
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setNumberDensity(double rho) {
  m_numberDensity = rho;
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
  double density;

  if (m_formula) {
    formula = Material::ChemicalFormula(*m_formula);
  } else if (m_atomicNo) {
    formula = createCompositionFromAtomicNumber();
  } else {
    throw std::runtime_error("MaterialBuilder::createNeutronAtom() - Please "
                             "specify one of chemical formula or atomic "
                             "number.");
  }

  density = getOrCalculateRho(formula);
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
  // first calculate the total number of atoms
  double totalNumAtoms = 0.;
  for (const auto &formulaUnit : formula) {
    totalNumAtoms += formulaUnit.multiplicity;
  }

  if (m_numberDensity) {
    return m_numberDensity.get();
  } else if (m_zParam && m_cellVol) {
    return totalNumAtoms * m_zParam.get() / m_cellVol.get();
  } else if (m_massDensity) {
    // g / cc -> atoms / Angstrom^3
    double rmm = 0.;
    for (const auto &formulaUnit : formula) {
      rmm += formulaUnit.atom->mass * formulaUnit.multiplicity;
    }
    return (m_massDensity.get() * totalNumAtoms / rmm) *
           PhysicalConstants::N_A * 1e-24;
  } else if (m_formula && m_formula->size() == 1) {
    return m_formula->at(0).atom->number_density;
  } else {
    throw std::runtime_error(
        "The number density could not be determined. Please "
        "provide the number density, ZParameter and unit "
        "cell volume or mass density.");
  }
}

bool MaterialBuilder::hasOverrideNeutronProperties() const {
  if (!isEmpty(m_totalXSection))
    return true;
  if (!isEmpty(m_cohXSection))
    return true;
  if (!isEmpty(m_incXSection))
    return true;
  if (!isEmpty(m_absSection))
    return true;
  return false;
}

PhysicalConstants::NeutronAtom MaterialBuilder::generateCustomNeutron() const {
  NeutronAtom neutronAtom(0, 0., 0., 0., 0., 0., 0.);

  // generate the default neutron
  if (m_atomicNo) {
    auto atom = getAtom(static_cast<uint16_t>(m_atomicNo.get()),
                        static_cast<uint16_t>(m_massNo));
    neutronAtom = atom.neutron;
  } else {
    double totalNumAtoms = 0.;
    for (const auto &formulaUnit : *m_formula) {
      neutronAtom =
          neutronAtom + formulaUnit.multiplicity * formulaUnit.atom->neutron;
      totalNumAtoms += formulaUnit.multiplicity;
    }
    neutronAtom = (1. / totalNumAtoms) * neutronAtom;
  }
  neutronAtom.a_number = 0; // signifies custom neutron atom
  neutronAtom.z_number = 0; // signifies custom neutron atom

  overrideNeutronProperties(neutronAtom);
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
