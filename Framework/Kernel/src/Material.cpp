#include "MantidKernel/Material.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/StringTokenizer.h"
#include <NeXusFile.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <numeric>

namespace Mantid {

namespace Kernel {
using tokenizer = Mantid::Kernel::StringTokenizer;
using str_pair = std::pair<std::string, std::string>;

using PhysicalConstants::Atom;
using PhysicalConstants::getAtom;
using PhysicalConstants::NeutronAtom;

namespace {
const double INV_FOUR_PI = 1. / (4. * M_PI);
}

Mantid::Kernel::Material::FormulaUnit::FormulaUnit(
    const boost::shared_ptr<PhysicalConstants::Atom> &atom,
    const double multiplicity)
    : atom(atom), multiplicity(multiplicity) {}

Mantid::Kernel::Material::FormulaUnit::FormulaUnit(
    const PhysicalConstants::Atom &atom, const double multiplicity)
    : atom(boost::make_shared<PhysicalConstants::Atom>(atom)),
      multiplicity(multiplicity) {}

/**
 * Construct an "empty" material. Everything returns zero
 */
Material::Material()
    : m_name(), m_chemicalFormula(), m_atomTotal(0.0), m_numberDensity(0.0),
      m_temperature(0.0), m_pressure(0.0) {}

/**
* Construct a material object
* @param name :: The name of the material
* @param formula :: The chemical formula
* @param numberDensity :: Density in atoms / Angstrom^3
* @param temperature :: The temperature in Kelvin (Default = 300K)
* @param pressure :: Pressure in kPa (Default: 101.325 kPa)
*/
Material::Material(const std::string &name, const ChemicalFormula &formula,
                   const double numberDensity, const double temperature,
                   const double pressure)
    : m_name(name), m_atomTotal(0.0), m_numberDensity(numberDensity),
      m_temperature(temperature), m_pressure(pressure) {
  m_chemicalFormula.assign(formula.begin(), formula.end());
  this->countAtoms();
}

/**
* Construct a material object
* @param name :: The name of the material
* @param atom :: The neutron atom to take scattering infrmation from
* @param numberDensity :: Density in atoms / Angstrom^3
* @param temperature :: The temperature in Kelvin (Default = 300K)
* @param pressure :: Pressure in kPa (Default: 101.325 kPa)
*/
Material::Material(const std::string &name,
                   const PhysicalConstants::NeutronAtom &atom,
                   const double numberDensity, const double temperature,
                   const double pressure)
    : m_name(name), m_chemicalFormula(), m_atomTotal(1.0),
      m_numberDensity(numberDensity), m_temperature(temperature),
      m_pressure(pressure) {
  if (atom.z_number == 0) { // user specified atom
    m_chemicalFormula.emplace_back(atom, 1.);
  } else if (atom.z_number > 0) { // single isotope
    m_chemicalFormula.emplace_back(getAtom(atom.z_number, atom.a_number), 1.);
  } else { // isotopic average
    m_chemicalFormula.emplace_back(atom, 1.);
  }
}
// update the total atom count
void Material::countAtoms() {
  m_atomTotal = std::accumulate(std::begin(m_chemicalFormula),
                                std::end(m_chemicalFormula), 0.,
                                [](double subtotal, const FormulaUnit &right) {
                                  return subtotal + right.multiplicity;
                                });
}

/**
 * Returns the name
 * @returns A string containing the name of the material
 */
const std::string &Material::name() const { return m_name; }

const Material::ChemicalFormula &Material::chemicalFormula() const {
  return m_chemicalFormula;
}

/**
 * Get the number density
 * @returns The number density of the material in atoms / Angstrom^3
 */
double Material::numberDensity() const { return m_numberDensity; }

/**
 * Get the temperature
 * @returns The temperature of the material in Kelvin
 */
double Material::temperature() const { return m_temperature; }

/**
 * Get the pressure
 * @returns The pressure of the material
 */
double Material::pressure() const { return m_pressure; }

/**
 * Get the coherent scattering cross section for a given wavelength.
 * CURRENTLY this simply returns the value for the underlying element
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the coherent scattering cross section at
 * the given wavelength
 */
double Material::cohScatterXSection(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.coh_scatt_xs * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/**
 * Get the incoherent scattering cross section for a given wavelength
 * CURRENTLY this simply returns the value for the underlying element
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the coherent scattering cross section at
 * the given wavelength
 */
double Material::incohScatterXSection(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.inc_scatt_xs * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/**
 * Get the total scattering cross section for a given wavelength
 * CURRENTLY this simply returns the value for sum of the incoherent
 * and coherent scattering cross sections.
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the total scattering cross section at
 * the given wavelength
 */
double Material::totalScatterXSection(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.tot_scatt_xs * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/**
 * Get the absorption cross section for a given wavelength.
 * CURRENTLY This assumes a linear dependence on the wavelength with the
 * reference
 * wavelength = NeutronAtom::ReferenceLambda angstroms.
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the absoprtion cross section at
 * the given wavelength
 */
double Material::absorbXSection(const double lambda) const {
  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.abs_scatt_xs * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal * (lambda / NeutronAtom::ReferenceLambda);
  }
}

double Material::cohScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.coh_scatt_length * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::incohScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.inc_scatt_length * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::cohScatterLengthReal(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula),
                      std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal +
                               right.atom->neutron.coh_scatt_length_real *
                                   right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::cohScatterLengthImg(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula),
                      std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal +
                               right.atom->neutron.coh_scatt_length_img *
                                   right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::incohScatterLengthReal(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula),
                      std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal +
                               right.atom->neutron.inc_scatt_length_real *
                                   right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::incohScatterLengthImg(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula),
                      std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal +
                               right.atom->neutron.inc_scatt_length_img *
                                   right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::totalScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  const double weightedTotal =
      std::accumulate(
          std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
          [](double subtotal, const FormulaUnit &right) {
            return subtotal +
                   right.atom->neutron.tot_scatt_length * right.multiplicity;
          }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

double Material::cohScatterLengthSqrd(const double lambda) const {
  const double weightedTotalReal = this->cohScatterLengthReal(lambda);
  const double weightedTotalImg = this->cohScatterLengthImg();

  if (!std::isnormal(weightedTotalReal)) {
    return 0.;
  } else {
    return (weightedTotalReal * weightedTotalReal) +
           (weightedTotalImg * weightedTotalImg);
  }
}

double Material::incohScatterLengthSqrd(const double lambda) const {
  const double weightedTotalReal = this->incohScatterLengthReal(lambda);
  const double weightedTotalImg = this->incohScatterLengthImg(lambda);

  if (!std::isnormal(weightedTotalReal)) {
    return 0.;
  } else {
    return (weightedTotalReal * weightedTotalReal) +
           (weightedTotalImg * weightedTotalImg);
  }
}

double Material::totalScatterLengthSqrd(const double lambda) const {
  // cross section has this properly averaged already
  double crossSection = totalScatterXSection(lambda);

  // 1 barn = 100 fm^2
  return 100. * crossSection * INV_FOUR_PI;
}

/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void Material::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXdata", true);
  file->putAttr("version", 2);
  file->putAttr("name", m_name);

  // determine how the information will be stored
  std::string style = "formula"; // default is a chemical formula
  if (m_chemicalFormula.empty()) {
    style = "empty";
  } else if (m_chemicalFormula.size() == 1) {
    if (m_chemicalFormula[0].atom->symbol == "user") {
      style = "userdefined";
    }
  }
  file->putAttr("formulaStyle", style);

  // write the actual information out
  if (style == "formula") {
    std::stringstream formula;
    for (const auto &formulaUnit : m_chemicalFormula) {
      if (formulaUnit.atom->a_number != 0) {
        formula << "(";
      }
      formula << formulaUnit.atom->symbol;
      if (formulaUnit.atom->a_number != 0) {
        formula << formulaUnit.atom->a_number << ")";
      }
      formula << formulaUnit.multiplicity << " ";
    }
    file->writeData("chemical_formula", formula.str());
  } else if (style == "userdefined") {
    file->writeData("coh_scatt_length_real",
                    m_chemicalFormula[0].atom->neutron.coh_scatt_length_real);
    file->writeData("coh_scatt_length_img",
                    m_chemicalFormula[0].atom->neutron.coh_scatt_length_img);
    file->writeData("inc_scatt_length_real",
                    m_chemicalFormula[0].atom->neutron.inc_scatt_length_real);
    file->writeData("inc_scatt_length_img",
                    m_chemicalFormula[0].atom->neutron.inc_scatt_length_img);
    file->writeData("coh_scatt_xs",
                    m_chemicalFormula[0].atom->neutron.coh_scatt_xs);
    file->writeData("inc_scatt_xs",
                    m_chemicalFormula[0].atom->neutron.inc_scatt_xs);
    file->writeData("tot_scatt_xs",
                    m_chemicalFormula[0].atom->neutron.tot_scatt_xs);
    file->writeData("abs_scatt_xs",
                    m_chemicalFormula[0].atom->neutron.abs_scatt_xs);
    file->writeData("tot_scatt_length",
                    m_chemicalFormula[0].atom->neutron.tot_scatt_length);
    file->writeData("coh_scatt_length",
                    m_chemicalFormula[0].atom->neutron.coh_scatt_length);
    file->writeData("inc_scatt_length",
                    m_chemicalFormula[0].atom->neutron.inc_scatt_length);
  }

  file->writeData("number_density", m_numberDensity);
  file->writeData("temperature", m_temperature);
  file->writeData("pressure", m_pressure);
  file->closeGroup();
}

/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open
 */
void Material::loadNexus(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXdata");
  file->getAttr("name", m_name);
  int version;
  file->getAttr("version", version);

  if (version == 1) {
    // Find the element
    uint16_t element_Z, element_A;
    file->readData("element_Z", element_Z);
    file->readData("element_A", element_A);
    try {
      m_chemicalFormula.clear();
      if (element_Z > 0) {
        m_chemicalFormula.emplace_back(getAtom(element_Z, element_A), 1);
      } else {
        m_chemicalFormula.emplace_back(
            Mantid::PhysicalConstants::getNeutronAtom(element_Z, element_A), 1);
      }
    } catch (std::runtime_error &) { /* ignore and use the default */
    }
  } else if (version == 2) {
    std::string style;
    file->getAttr("formulaStyle", style);

    if (style == "formula") {
      std::string formula;
      file->readData("chemical_formula", formula);
      this->m_chemicalFormula = Material::parseChemicalFormula(formula);
      this->countAtoms();
    } else if (style == "userdefined") {
      NeutronAtom neutron;
      file->readData("coh_scatt_length_real", neutron.coh_scatt_length_real);
      file->readData("coh_scatt_length_img", neutron.coh_scatt_length_img);
      file->readData("inc_scatt_length_real", neutron.inc_scatt_length_real);
      file->readData("inc_scatt_length_img", neutron.inc_scatt_length_img);
      file->readData("coh_scatt_xs", neutron.coh_scatt_xs);
      file->readData("inc_scatt_xs", neutron.inc_scatt_xs);
      file->readData("tot_scatt_xs", neutron.tot_scatt_xs);
      file->readData("abs_scatt_xs", neutron.abs_scatt_xs);
      file->readData("tot_scatt_length", neutron.tot_scatt_length);
      file->readData("coh_scatt_length", neutron.coh_scatt_length);
      file->readData("inc_scatt_length", neutron.inc_scatt_length);

      m_chemicalFormula.emplace_back(boost::make_shared<Atom>(neutron), 1);
    }
    // the other option is empty which does not need to be addressed
  } else {
    throw std::runtime_error(
        "Only know how to read version 1 or 2 for Material");
  }
  this->countAtoms();

  file->readData("number_density", m_numberDensity);
  file->readData("temperature", m_temperature);
  file->readData("pressure", m_pressure);
  file->closeGroup();
}

namespace { // anonymous namespace to hide the function
str_pair
getAtomName(const std::string &text) // TODO change to get number after letters
{
  // one character doesn't need
  if (text.size() <= 1)
    return std::make_pair(text, "");

  // check the second character
  const char *s;
  s = text.c_str();
  if ((s[1] >= '0' && s[1] <= '9') || s[1] == '.')
    return std::make_pair(text.substr(0, 1), text.substr(1));
  else
    return std::make_pair(text.substr(0, 2), text.substr(2));
}
}

Material::ChemicalFormula
Material::parseChemicalFormula(const std::string chemicalSymbol) {
  Material::ChemicalFormula CF;

  tokenizer tokens(chemicalSymbol, " -",
                   Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  for (const auto &atom : tokens) {
    try {
      std::string name;
      float numberAtoms = 1;
      uint16_t aNumber = 0;

      // split out the isotope bit
      if (atom.find('(') != std::string::npos) {
        // error check
        size_t end = atom.find(')');
        if (end == std::string::npos) {
          std::stringstream msg;
          msg << "Failed to parse isotope \"" << atom << "\"";
          throw std::runtime_error(msg.str());
        }

        // get the number of atoms
        std::string numberAtomsStr = atom.substr(end + 1);
        if (!numberAtomsStr.empty())
          numberAtoms = boost::lexical_cast<float>(numberAtomsStr);

        // split up the atom and isotope number
        name = atom.substr(1, end - 1);
        str_pair temp = getAtomName(name);

        name = temp.first;
        aNumber = boost::lexical_cast<uint16_t>(temp.second);
      } else // for non-isotopes
      {
        str_pair temp = getAtomName(atom);
        name = temp.first;
        if (!temp.second.empty())
          numberAtoms = boost::lexical_cast<float>(temp.second);
      }

      CF.emplace_back(getAtom(name, aNumber), static_cast<double>(numberAtoms));
    } catch (boost::bad_lexical_cast &e) {
      std::stringstream msg;
      msg << "While trying to parse atom \"" << atom
          << "\" encountered bad_lexical_cast: " << e.what();
      throw std::runtime_error(msg.str());
    }
  }

  return CF;
}
}
}
