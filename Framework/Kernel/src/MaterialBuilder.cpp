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
      m_totalXSection(), m_cohXSection(), m_incXSection(), m_absSection() {}

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
 * @param name Human-readable name of the material
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setFormula(const std::string &formula) {
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
 * @param Number density of the sample
 * @return A reference to the this object to allow chaining
 */
MaterialBuilder &MaterialBuilder::setNumberDensity(double rho) {
  m_numberDensity = rho;
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
  if (m_formula && m_atomicNo) {
    throw std::runtime_error("MaterialBuilder::createNeutronAtom() - Both a "
                             "formula and atomic number have been specified "
                             "please only provide one.");
  } else if (m_formula) {
    std::tie(neutron, density) = createCompositionFromFormula();
  } else if (m_atomicNo) {
    std::tie(neutron, density) = createCompositionFromAtomicNumber();
  } else {
    throw std::runtime_error("MaterialBuilder::createNeutronAtom() - Please "
                             "specify one of chemical formula or atomic "
                             "number.");
  }
  overrideNeutronProperties(neutron);
  if (m_numberDensity) {
    density = m_numberDensity.get();
  }
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
  double density(EMPTY_DBL());
  const size_t numAtomTypes(m_formula->atoms.size());
  if (numAtomTypes == 1) {
    composition = m_formula->atoms[0]->neutron;
    density = m_formula->atoms[0]->number_density;
  } else {
    double totalNumAtoms(0.0), rmm(0.0);
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
  return Composition{atom.neutron, atom.number_density};
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
