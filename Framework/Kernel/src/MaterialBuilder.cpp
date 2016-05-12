#include "MantidKernel/MaterialBuilder.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
using PhysicalConstants::Atom;
using PhysicalConstants::NeutronAtom;
using PhysicalConstants::getAtom;
using PhysicalConstants::getNeutronAtom;
namespace Kernel {

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
 * Set the checmical formula of the material
 * @param formula Human-readable name of the material
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setFormula(const std::string &formula) {
  if (m_atomicNo) {
    throw std::runtime_error("MaterialBuilder::setFormula() - Atomic no. "
                             "already set, cannot use formula aswell.");
  }
  if (formula.empty()) {
    throw std::invalid_argument(
        "MaterialBuilder::setFormula() - Empty formula provided.");
  }
  typedef Material::ChemicalFormula ChemicalFormula;
  try {
    m_formula = Mantid::Kernel::make_unique<ChemicalFormula>(
        ChemicalFormula(Material::parseChemicalFormula(formula)));
  } catch (std::runtime_error &ex) {
    throw std::invalid_argument(
        "MaterialBuilder::setFormula() - Unable to parse chemical formula.");
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
 * Set the number density of the sample
 * @param rho density of the sample
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setNumberDensity(double rho) {
  m_numberDensity = rho;
  return *this;
}

/**
 * Set the number of atoms in the unit cell
 * @param zparam Number of atoms
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
 * Set the mass density of the sample and calculate the density from this
 * @param massDensity The mass density value
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
  NeutronAtom neutron;
  double density(0.0);
  if (m_formula) {
    std::tie(neutron, density) = createCompositionFromFormula();
  } else if (m_atomicNo) {
    std::tie(neutron, density) = createCompositionFromAtomicNumber();
  } else {
    throw std::runtime_error("MaterialBuilder::createNeutronAtom() - Please "
                             "specify one of chemical formula or atomic "
                             "number.");
  }
  overrideNeutronProperties(neutron);
  return Material(m_name, neutron, density);
}

/**
 * Create the NeutronAtom object from a chemical formula
 * @return A new NeutronAtom object with the defined proprties
 */
MaterialBuilder::Composition
MaterialBuilder::createCompositionFromFormula() const {
  // The zeroes here are important. By default the entries are all nan
  NeutronAtom composition(0, 0., 0., 0., 0., 0., 0.);
  const size_t numAtomTypes(m_formula->atoms.size());
  double totalNumAtoms(0.0), rmm(0.0);
  if (numAtomTypes == 1) {
    composition = m_formula->atoms[0]->neutron;
    totalNumAtoms = 1.0;
    rmm = m_formula->atoms[0]->mass;
  } else {
    for (size_t i = 0; i < numAtomTypes; i++) {
      const auto &atom(m_formula->atoms[i]);
      const double natoms(m_formula->numberAtoms[i]);
      composition = composition + natoms * atom->neutron;
      totalNumAtoms += natoms;
      rmm += atom->mass * natoms;
    }
    // normalize the accumulated number by the number of atoms
    composition = (1. / totalNumAtoms) * composition;
  }
  const double density = getOrCalculateRho(totalNumAtoms, rmm);
  return Composition{composition, density};
}

/**
 * Create the NeutronAtom object from the atomic number
 * @return A new NeutronAtom object with the defined proprties
 */
MaterialBuilder::Composition
MaterialBuilder::createCompositionFromAtomicNumber() const {
  auto atom = getAtom(static_cast<uint16_t>(m_atomicNo.get()),
                      static_cast<uint16_t>(m_massNo));
  return Composition{atom.neutron, getOrCalculateRho(1.0, atom.mass)};
}

/**
 * Return the manually set density or calculate it from other parameters
 * @param totalNumAtoms Total number of atoms
 * @param rmm The relative molecular mass
 * @return The number density
 */
double MaterialBuilder::getOrCalculateRho(double totalNumAtoms,
                                          double rmm) const {
  if (m_numberDensity) {
    return m_numberDensity.get();
  } else if (m_zParam && m_cellVol) {
    return totalNumAtoms * m_zParam.get() / m_cellVol.get();
  } else if (m_massDensity) {
    return (m_massDensity.get() / rmm) * PhysicalConstants::N_A * 1e-24;
  } else if (m_formula && m_formula->atoms.size() == 1) {
    return m_formula->atoms[0]->number_density;
  } else {
    return EMPTY_DBL();
  }
}

/**
 * Override default neutron properties with those supplied
 * @param neutron A reference to a NeutronAtom object
 */
void MaterialBuilder::overrideNeutronProperties(
    PhysicalConstants::NeutronAtom &neutron) const {
  if (m_totalXSection)
    neutron.tot_scatt_xs = m_totalXSection.get();
  if (m_cohXSection)
    neutron.coh_scatt_xs = m_cohXSection.get();
  if (m_incXSection)
    neutron.inc_scatt_xs = m_incXSection.get();
  if (m_absSection)
    neutron.abs_scatt_xs = m_absSection.get();
}

} // namespace Kernel
} // namespace Mantid
