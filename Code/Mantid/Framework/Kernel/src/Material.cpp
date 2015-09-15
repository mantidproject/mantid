//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidKernel/Material.h"
#include "MantidKernel/Atom.h"
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

namespace Mantid {

namespace Kernel {
typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
typedef std::pair<std::string, std::string> str_pair;

using PhysicalConstants::Atom;
using PhysicalConstants::getAtom;
using PhysicalConstants::NeutronAtom;

/**
 * Construct an "empty" material. Everything returns zero
 */
Material::Material()
    : m_name(), m_element(0, 0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0),
      m_numberDensity(0.0), m_temperature(0.0), m_pressure(0.0) {}

/**
* Construct a material object
* @param name :: The name of the material
* @param element :: The element it is composed from
* @param numberDensity :: Density in A^-3
* @param temperature :: The temperature in Kelvin (Default = 300K)
* @param pressure :: Pressure in kPa (Default: 101.325 kPa)
*/
Material::Material(const std::string &name,
                   const PhysicalConstants::NeutronAtom &element,
                   const double numberDensity, const double temperature,
                   const double pressure)
    : m_name(name), m_element(element), m_numberDensity(numberDensity),
      m_temperature(temperature), m_pressure(pressure) {}

/**
 * Returns the name
 * @returns A string containing the name of the material
 */
const std::string &Material::name() const { return m_name; }

/**
 * Get the number density
 * @returns The number density of the material in A^-3
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
  (void)lambda;
  return m_element.coh_scatt_xs;
}

/**
 * Get the incoherent scattering cross section for a given wavelength
 * CURRENTLY this simply returns the value for the underlying element
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the coherent scattering cross section at
 * the given wavelength
 */
double Material::incohScatterXSection(const double lambda) const {
  (void)lambda;
  return m_element.inc_scatt_xs;
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
  return m_element.tot_scatt_xs;
}

/**
 * Get the absorption cross section for a given wavelength.
 * CURRENTLY This assumes a linear dependence on the wavelength with the
 * reference
 * wavelength = NeutronAtom::ReferenceLambda angstroms.
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the absoprtioncross section at
 * the given wavelength
 */
double Material::absorbXSection(const double lambda) const {
  return (m_element.abs_scatt_xs) * (lambda / NeutronAtom::ReferenceLambda);
}

/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void Material::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXdata", 1);
  file->putAttr("version", 1);
  file->putAttr("name", m_name);
  file->writeData("element_Z", m_element.z_number);
  file->writeData("element_A", m_element.a_number);
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

  // Find the element
  uint16_t element_Z, element_A;
  file->readData("element_Z", element_Z);
  file->readData("element_A", element_A);
  try {
    m_element = Mantid::PhysicalConstants::getNeutronAtom(element_Z, element_A);
  } catch (std::runtime_error &) { /* ignore and use the default */
  }

  file->readData("number_density", m_numberDensity);
  file->readData("temperature", m_temperature);
  file->readData("pressure", m_pressure);
  file->closeGroup();
}

namespace { // anonymous namespace to hide the function
str_pair
getAtomName(std::string &text) // TODO change to get number after letters
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

  const boost::char_separator<char> ATOM_DELIM(" -");
  tokenizer tokens(chemicalSymbol, ATOM_DELIM);
  for (auto atom = tokens.begin(); atom != tokens.end(); ++atom) {
    try {
      std::string name(*atom);
      float numberAtoms = 1;
      uint16_t aNumber = 0;

      // split out the isotope bit
      if (atom->find('(') != std::string::npos) {
        // error check
        size_t end = atom->find(')');
        if (end == std::string::npos) {
          std::stringstream msg;
          msg << "Failed to parse isotope \"" << name << "\"";
          throw std::runtime_error(msg.str());
        }

        // get the number of atoms
        std::string numberAtomsStr = name.substr(end + 1);
        if (!numberAtomsStr.empty())
          numberAtoms = boost::lexical_cast<float>(numberAtomsStr);

        // split up the atom and isotope number
        name = name.substr(1, end - 1);
        str_pair temp = getAtomName(name);

        name = temp.first;
        aNumber = boost::lexical_cast<uint16_t>(temp.second);
      } else // for non-isotopes
      {
        str_pair temp = getAtomName(name);
        name = temp.first;
        if (!temp.second.empty())
          numberAtoms = boost::lexical_cast<float>(temp.second);
      }

      CF.atoms.push_back(boost::make_shared<Atom>(getAtom(name, aNumber)));
      CF.numberAtoms.push_back(numberAtoms);
    } catch (boost::bad_lexical_cast &e) {
      std::stringstream msg;
      msg << "While trying to parse atom \"" << (*atom)
          << "\" encountered bad_lexical_cast: " << e.what();
      throw std::runtime_error(msg.str());
    }
  }

  return CF;
}
}
}
