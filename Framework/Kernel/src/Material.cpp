// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Material.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/AttenuationProfile.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidNexusCpp/NeXusFile.hpp"
#include <boost/lexical_cast.hpp>
#include <memory>
#include <numeric>
#include <utility>

namespace Mantid::Kernel {
using tokenizer = Mantid::Kernel::StringTokenizer;
using str_pair = std::pair<std::string, std::string>;

using PhysicalConstants::Atom;
using PhysicalConstants::getAtom;
using PhysicalConstants::NeutronAtom;

namespace {
constexpr double INV_FOUR_PI = 1. / (4. * M_PI);

inline double scatteringLength(const double real, const double imag) {
  double length;
  if (imag == 0.) {
    length = std::abs(real);
  } else if (real == 0.) {
    length = std::abs(imag);
  } else {
    length = std::hypot(real, imag);
  }

  if (!std::isnormal(length)) {
    return 0.;
  } else {
    return length;
  }
}

inline double scatteringXS(const double realLength, const double imagLength) {
  double lengthSqrd = (realLength * realLength) + (imagLength * imagLength);

  if (!std::isnormal(lengthSqrd)) {
    return 0.;
  } else {
    return .04 * M_PI * lengthSqrd;
  }
}
} // namespace

Mantid::Kernel::Material::FormulaUnit::FormulaUnit(std::shared_ptr<PhysicalConstants::Atom> atom,
                                                   const double multiplicity)
    : atom(std::move(atom)), multiplicity(multiplicity) {}

Mantid::Kernel::Material::FormulaUnit::FormulaUnit(const PhysicalConstants::Atom &atom, const double multiplicity)
    : atom(std::make_shared<PhysicalConstants::Atom>(atom)), multiplicity(multiplicity) {}

/**
 * Construct an "empty" material. Everything returns zero
 */
Material::Material()
    : m_name(), m_chemicalFormula(), m_atomTotal(0.0), m_numberDensity(0.0), m_packingFraction(1.0), m_temperature(0.0),
      m_pressure(0.0), m_linearAbsorpXSectionByWL(0.0), m_totalScatterXSection(0.0) {}

/**
 * Construct a material object
 * @param name :: The name of the material
 * @param formula :: The chemical formula
 * @param numberDensity :: Density in atoms / Angstrom^3
 * @param packingFraction :: Packing fraction of material
 * @param temperature :: The temperature in Kelvin (Default = 300K)
 * @param pressure :: Pressure in kPa (Default: 101.325 kPa)
 */
Material::Material(std::string name, const ChemicalFormula &formula, const double numberDensity,
                   const double packingFraction, const double temperature, const double pressure)
    : m_name(std::move(name)), m_atomTotal(0.0), m_numberDensity(numberDensity), m_packingFraction(packingFraction),
      m_temperature(temperature), m_pressure(pressure) {
  m_chemicalFormula.assign(formula.begin(), formula.end());
  this->countAtoms();
  this->calculateLinearAbsorpXSectionByWL();
  this->calculateTotalScatterXSection();
}

/**
 * Construct a material object
 * @param name :: The name of the material
 * @param atom :: The neutron atom to take scattering infrmation from
 * @param numberDensity :: Density in atoms / Angstrom^3
 * @param packingFraction :: Packing fraction of material
 * @param temperature :: The temperature in Kelvin (Default = 300K)
 * @param pressure :: Pressure in kPa (Default: 101.325 kPa)
 */
Material::Material(std::string name, const PhysicalConstants::NeutronAtom &atom, const double numberDensity,
                   const double packingFraction, const double temperature, const double pressure)
    : m_name(std::move(name)), m_chemicalFormula(), m_atomTotal(1.0), m_numberDensity(numberDensity),
      m_packingFraction(packingFraction), m_temperature(temperature), m_pressure(pressure) {
  if (atom.z_number == 0) { // user specified atom
    m_chemicalFormula.emplace_back(atom, 1.);
  } else if (atom.a_number > 0) { // single isotope
    m_chemicalFormula.emplace_back(getAtom(atom.z_number, atom.a_number), 1.);
  } else { // isotopic average
    m_chemicalFormula.emplace_back(atom, 1.);
  }
  this->calculateLinearAbsorpXSectionByWL();
  this->calculateTotalScatterXSection();
}

// update the total atom count
void Material::countAtoms() {
  m_atomTotal =
      std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) { return subtotal + right.multiplicity; });
}

/**
 * Calculate the absorption cross section for a given wavelength
 * according to Sears eqn 14. Store result as a cross section per wavelength
 * to enable the result to be reused to calculate the cross section for
 * specific wavelengths (assuming linear dependence on the wavelength)
 * with the reference wavelength = NeutronAtom::ReferenceLambda angstroms.
 */

void Material::calculateLinearAbsorpXSectionByWL() {
  double weightedTotal;

  if (m_chemicalFormula.size() == 1) {
    weightedTotal = m_chemicalFormula.front().atom->neutron.abs_scatt_xs;
  } else {
    weightedTotal = std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                                    [](double subtotal, const FormulaUnit &right) {
                                      return subtotal + right.atom->neutron.abs_scatt_xs * right.multiplicity;
                                    }) /
                    m_atomTotal;
  }

  if (!std::isnormal(weightedTotal)) {
    weightedTotal = 0.;
  }

  m_linearAbsorpXSectionByWL = weightedTotal / PhysicalConstants::NeutronAtom::ReferenceLambda;
}

// calculate the total scattering x section (by wavelength) following Sears
// eqn 13.
void Material::calculateTotalScatterXSection() {
  double weightedTotal;
  if (m_chemicalFormula.size() == 1)
    weightedTotal = m_chemicalFormula.front().atom->neutron.tot_scatt_xs;
  else {
    weightedTotal = std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                                    [](double subtotal, const FormulaUnit &right) {
                                      return subtotal + right.atom->neutron.tot_scatt_xs * right.multiplicity;
                                    }) /
                    m_atomTotal;
  }

  if (!std::isnormal(weightedTotal)) {
    m_totalScatterXSection = 0.;
  } else {
    m_totalScatterXSection = weightedTotal;
  }
}

void Material::setAttenuationProfile(AttenuationProfile attenuationOverride) {
  m_attenuationOverride = std::move(attenuationOverride);
}

void Material::setXRayAttenuationProfile(AttenuationProfile attenuationProfile) {
  m_xRayAttenuationProfile = std::move(attenuationProfile);
}

/**
 * Returns the name
 * @returns A string containing the name of the material
 */
const std::string &Material::name() const { return m_name; }

const Material::ChemicalFormula &Material::chemicalFormula() const { return m_chemicalFormula; }

/**
 * Get the number density
 * @returns The number density of the material in atoms / Angstrom^3
 */
double Material::numberDensity() const { return m_numberDensity; }

/**
 * Get the effective number density
 * @returns The number density of the material in atoms / Angstrom^3
 */
double Material::numberDensityEffective() const { return m_numberDensity * m_packingFraction; }

/**
 * Get the packing fraction. This should be a number 0<f<=1. However,
 * this is sometimes used as a fudge factor and is allowed to vary 0<f<2.
 * @returns The packing fraction
 */
double Material::packingFraction() const { return m_packingFraction; }

/**
 * The total number of atoms in the chemical formula. This is commonly
 * used to convert multiplicity into relative values.
 * @return The total number of atoms
 */
double Material::totalAtoms() const { return m_atomTotal; }

/**
 * Get the temperature
 * @returns The temperature of the material in Kelvin
 */
double Material::temperature() const { return m_temperature; }

/**
 * Get the pressure
 * @returns The pressure of the material, in kPa (Default: 101.325 kPa)
 */
double Material::pressure() const { return m_pressure; }

/**
 * Get the coherent scattering cross section according to Sears eqn 7.
 *
 * @returns The value of the coherent scattering cross section.
 */
double Material::cohScatterXSection() const {
  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.coh_scatt_xs;

  return scatteringXS(cohScatterLengthReal(), cohScatterLengthImg());
}

/**
 * Get the incoherent scattering cross section according to Sears eqn 16
 *
 * @returns The value of the coherent scattering cross section.
 */
double Material::incohScatterXSection() const {
  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.inc_scatt_xs;

  return totalScatterXSection() - cohScatterXSection();
}

/**
 * Get the total scattering cross section
 *
 * @returns The value of the total scattering cross section.
 */
double Material::totalScatterXSection() const { return m_totalScatterXSection; }

/**
 * Get the absorption cross section for a given wavelength
 * @param lambda :: The wavelength to evaluate the cross section
 * @returns The value of the absoprtion cross section at
 * the given wavelength
 */
double Material::absorbXSection(const double lambda) const { return m_linearAbsorpXSectionByWL * lambda; }

/**
 * @param lambda Wavelength (Angstroms) to compute the attenuation (default =
 * reference lambda)
 * @return The attenuation coefficient in m-1
 */
double Material::attenuationCoefficient(const double lambda) const {
  if (!m_attenuationOverride) {
    return 100 * numberDensityEffective() * (totalScatterXSection() + absorbXSection(lambda));
  } else {
    return m_attenuationOverride->getAttenuationCoefficient(lambda);
  }
}

/**
 * @param distance Distance (m) travelled
 * @param lambda Wavelength (Angstroms) to compute the attenuation (default =
 * reference lambda)
 * @return The dimensionless attenuation factor
 */
double Material::attenuation(const double distance, const double lambda) const {
  return exp(-attenuationCoefficient(lambda) * distance);
}

/**
 * @param distance Distance (m) travelled
 * @param energy KeV to compute the attenuation
 * @return The dimensionless attenuation factor
 */
double Material::xRayAttenuation(const double distance, const double energy) const {
  if (m_xRayAttenuationProfile) {
    return exp(-m_xRayAttenuationProfile->getAttenuationCoefficient(energy) * distance);
  } else {
    throw std::runtime_error("xRayAttenuationProfile override not set");
  }
}
/*
 * @returns true if m_xRayAttenuationOverride is set and false if not
 */
bool Material::hasValidXRayAttenuationProfile() {
  if (m_xRayAttenuationProfile) {
    return true;
  } else {
    return false;
  }
}
// NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
// will cancel for mu to give units: cm^-1
double Material::linearAbsorpCoef(const double lambda) const {
  return absorbXSection(lambda) * 100. * numberDensityEffective();
}

// This must match the values that come from the scalar version
std::vector<double> Material::linearAbsorpCoef(std::vector<double>::const_iterator lambdaBegin,
                                               std::vector<double>::const_iterator lambdaEnd) const {

  const double densityTerm = 100. * numberDensityEffective();

  std::vector<double> linearCoef(std::distance(lambdaBegin, lambdaEnd));

  std::transform(lambdaBegin, lambdaEnd, linearCoef.begin(),
                 [densityTerm, this](const double lambda) { return densityTerm * this->absorbXSection(lambda); });

  return linearCoef;
}

/// According to Sears eqn 12
double Material::cohScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.coh_scatt_length;

  // these have already accounted for single atom case
  return scatteringLength(cohScatterLengthReal(), cohScatterLengthImg());
}

/// According to Sears eqn 7
double Material::incohScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.inc_scatt_length;

  return scatteringLength(incohScatterLengthReal(), incohScatterLengthImg());
}

/// Sears eqn 12
double Material::cohScatterLengthReal(const double lambda) const {
  UNUSED_ARG(lambda);
  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.coh_scatt_length_real;

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal + right.atom->neutron.coh_scatt_length_real * right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/// Sears eqn 12
double Material::cohScatterLengthImg(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.coh_scatt_length_img;

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal + right.atom->neutron.coh_scatt_length_img * right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/// Not explicitly in Sears, but following eqn 12
double Material::incohScatterLengthReal(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.inc_scatt_length_real;

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal + right.atom->neutron.inc_scatt_length_real * right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/// Not explicitly in Sears, but following eqn 12
double Material::incohScatterLengthImg(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.inc_scatt_length_img;

  const double weightedTotal =
      std::accumulate(std::begin(m_chemicalFormula), std::end(m_chemicalFormula), 0.,
                      [](double subtotal, const FormulaUnit &right) {
                        return subtotal + right.atom->neutron.inc_scatt_length_img * right.multiplicity;
                      }) /
      m_atomTotal;

  if (!std::isnormal(weightedTotal)) {
    return 0.;
  } else {
    return weightedTotal;
  }
}

/// Sears eqn 13
double Material::totalScatterLength(const double lambda) const {
  UNUSED_ARG(lambda);

  if (m_chemicalFormula.size() == 1)
    return m_chemicalFormula.front().atom->neutron.tot_scatt_length;

  const double crossSection = totalScatterXSection();
  return 10. * std::sqrt(crossSection) * INV_FOUR_PI;
}

double Material::cohScatterLengthSqrd(const double lambda) const {
  UNUSED_ARG(lambda);

  // these have already acconted for single atom case
  const double real = this->cohScatterLengthReal();
  const double imag = this->cohScatterLengthImg();

  double lengthSqrd;
  if (imag == 0.) {
    lengthSqrd = real * real;
  } else if (real == 0.) {
    lengthSqrd = imag * imag;
  } else {
    lengthSqrd = real * real + imag * imag;
  }

  if (!std::isnormal(lengthSqrd)) {
    return 0.;
  } else {
    return lengthSqrd;
  }
}

double Material::incohScatterLengthSqrd(const double lambda) const {
  UNUSED_ARG(lambda);

  // cross section has this properly averaged already
  const double crossSection = incohScatterXSection();

  // 1 barn = 100 fm^2
  return 100. * crossSection * INV_FOUR_PI;
}

double Material::totalScatterLengthSqrd(const double lambda) const {
  UNUSED_ARG(lambda);

  // cross section has this properly averaged already
  const double crossSection = totalScatterXSection();

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
    file->writeData("coh_scatt_length_real", m_chemicalFormula[0].atom->neutron.coh_scatt_length_real);
    file->writeData("coh_scatt_length_img", m_chemicalFormula[0].atom->neutron.coh_scatt_length_img);
    file->writeData("inc_scatt_length_real", m_chemicalFormula[0].atom->neutron.inc_scatt_length_real);
    file->writeData("inc_scatt_length_img", m_chemicalFormula[0].atom->neutron.inc_scatt_length_img);
    file->writeData("coh_scatt_xs", m_chemicalFormula[0].atom->neutron.coh_scatt_xs);
    file->writeData("inc_scatt_xs", m_chemicalFormula[0].atom->neutron.inc_scatt_xs);
    file->writeData("tot_scatt_xs", m_chemicalFormula[0].atom->neutron.tot_scatt_xs);
    file->writeData("abs_scatt_xs", m_chemicalFormula[0].atom->neutron.abs_scatt_xs);
    file->writeData("tot_scatt_length", m_chemicalFormula[0].atom->neutron.tot_scatt_length);
    file->writeData("coh_scatt_length", m_chemicalFormula[0].atom->neutron.coh_scatt_length);
    file->writeData("inc_scatt_length", m_chemicalFormula[0].atom->neutron.inc_scatt_length);
  }

  file->writeData("number_density", m_numberDensity);
  file->writeData("packing_fraction", m_packingFraction);
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
        m_chemicalFormula.emplace_back(Mantid::PhysicalConstants::getNeutronAtom(element_Z, element_A), 1);
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

      m_chemicalFormula.emplace_back(std::make_shared<Atom>(neutron), 1);
    }
    // the other option is empty which does not need to be addressed
  } else {
    throw std::runtime_error("Only know how to read version 1 or 2 for Material");
  }
  this->countAtoms();
  this->calculateLinearAbsorpXSectionByWL();
  this->calculateTotalScatterXSection();

  file->readData("number_density", m_numberDensity);
  try {
    file->readData("packing_fraction", m_packingFraction);
  } catch (std::runtime_error &) {
    m_packingFraction = 1.;
  }
  file->readData("temperature", m_temperature);
  file->readData("pressure", m_pressure);
  file->closeGroup();
}

namespace {                                   // anonymous namespace to hide the function
str_pair getAtomName(const std::string &text) // TODO change to get number after letters
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
} // namespace

Material::ChemicalFormula Material::parseChemicalFormula(const std::string &chemicalSymbol) {
  Material::ChemicalFormula CF;

  tokenizer tokens(chemicalSymbol, " -", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  for (const auto &atom : tokens) {
    try {
      std::string atomName;
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
        atomName = atom.substr(1, end - 1);
        str_pair temp = getAtomName(atomName);

        atomName = temp.first;
        aNumber = boost::lexical_cast<uint16_t>(temp.second);
      } else // for non-isotopes
      {
        str_pair temp = getAtomName(atom);
        atomName = temp.first;
        if (!temp.second.empty())
          numberAtoms = boost::lexical_cast<float>(temp.second);
      }

      CF.emplace_back(getAtom(atomName, aNumber), static_cast<double>(numberAtoms));
    } catch (boost::bad_lexical_cast &e) {
      std::stringstream msg;
      msg << "While trying to parse atom \"" << atom << "\" encountered bad_lexical_cast: " << e.what();
      throw std::runtime_error(msg.str());
    }
  }

  return CF;
}
} // namespace Mantid::Kernel
