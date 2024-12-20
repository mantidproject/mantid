// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidJson/Json.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/StringTokenizer.h"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <json/json.h>
#include <stdexcept>
#include <utility>

namespace Mantid::Geometry {

using namespace Kernel;

/// Constructor which takes an element symbol, fractional coordinates, isotropic
/// atomic displacement parameter and occupancy.
IsotropicAtomBraggScatterer::IsotropicAtomBraggScatterer() : BraggScattererInCrystalStructure(), m_atom(), m_label() {}

/// Clones the instance.
BraggScatterer_sptr IsotropicAtomBraggScatterer::clone() const {
  IsotropicAtomBraggScatterer_sptr clone = std::make_shared<IsotropicAtomBraggScatterer>();
  clone->initialize();
  clone->setProperties(this->asString(false));

  return clone;
}

/// Tries to obtain element specific data for the given symbol using
/// PhysicalConstants::getAtom.
void IsotropicAtomBraggScatterer::setElement(const std::string &element) {
  PhysicalConstants::Atom atom = PhysicalConstants::getAtom(element);

  m_atom = atom.neutron;
  m_label = element;
}

/// Returns the string representation of the contained element.
std::string IsotropicAtomBraggScatterer::getElement() const { return m_label; }

/// Returns the internally stored NeutronAtom that holds element specific data.
PhysicalConstants::NeutronAtom IsotropicAtomBraggScatterer::getNeutronAtom() const { return m_atom; }

/// Returns the occupancy.
double IsotropicAtomBraggScatterer::getOccupancy() const { return getProperty("Occupancy"); }

/// Returns the isotropic atomic displacement parameter.
double IsotropicAtomBraggScatterer::getU() const { return getProperty("U"); }

/**
 * Calculates the structure factor
 *
 * This method calculates the structure factor.
 * For details, please refer to the class documentation in the header file.
 *
 * @param hkl :: HKL for which the structure factor should be calculated
 * @return Structure factor (complex).
 */
StructureFactor IsotropicAtomBraggScatterer::calculateStructureFactor(const V3D &hkl) const {
  double amplitude = getOccupancy() * getDebyeWallerFactor(hkl) * getScatteringLength();

  double phase = 2.0 * M_PI * m_position.scalar_prod(hkl);

  return amplitude * StructureFactor(cos(phase), sin(phase));
}

/**
 * Declares properties of this scatterer model
 *
 * In addition to the properties of BraggScatterer, this class implements three
 *more properties,
 * as described in the general class documentation, with some restrictions on
 *allowed
 * values:
 *  - U must be 0 or greater
 *  - Occupancy must be on the interval [0,1]
 *  - Element must be present.
 */
void IsotropicAtomBraggScatterer::declareScattererProperties() {
  // Default behavior requires this.
  setElement("H");

  std::shared_ptr<BoundedValidator<double>> uValidator = std::make_shared<BoundedValidator<double>>();
  uValidator->setLower(0.0);

  declareProperty(std::make_unique<PropertyWithValue<double>>("U", 0.0, uValidator),
                  "Isotropic atomic displacement in Angstrom^2");

  IValidator_sptr occValidator = std::make_shared<BoundedValidator<double>>(0.0, 1.0);
  declareProperty(std::make_unique<PropertyWithValue<double>>("Occupancy", 1.0, occValidator),
                  "Site occupancy, values on interval [0,1].");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>(
      "Element", "H", std::make_shared<MandatoryValidator<std::string>>()));
}

/// After setting the element as a string, the corresponding
void IsotropicAtomBraggScatterer::afterScattererPropertySet(const std::string &propertyName) {
  if (propertyName == "Element") {
    setElement(getPropertyValue(propertyName));
  }
}

/// Returns the Debye-Waller factor, using an isotropic atomic displacement and
/// the stored unit cell.
double IsotropicAtomBraggScatterer::getDebyeWallerFactor(const V3D &hkl) const {
  V3D dstar = getCell().getB() * hkl;

  return exp(-2.0 * M_PI * M_PI * getU() * dstar.norm2());
}

/// Returns the scattering length of the stored element.
double IsotropicAtomBraggScatterer::getScatteringLength() const { return m_atom.coh_scatt_length_real; }

DECLARE_BRAGGSCATTERER(IsotropicAtomBraggScatterer)

/**
 * Constructor for vector with IsotropicAtomBraggScatterers from a string
 *
 * The functor expects to be constructed from a string in the following format:
 *
 *  Element x y z occupancy u_iso; Element x y z occupancy u_iso; ...
 *
 * It generates an IsotropicAtomBraggScatterer for each specified atom.
 *
 * @param scattererString :: String in the format specified above
 */
IsotropicAtomBraggScattererParser::IsotropicAtomBraggScattererParser(std::string scattererString)
    : m_scattererString(std::move(scattererString)) {}

/// Operator that returns vector of IsotropicAtomBraggScatterers.
std::vector<BraggScatterer_sptr> IsotropicAtomBraggScattererParser::operator()() const {
  Mantid::Kernel::StringTokenizer tokens(m_scattererString, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  std::vector<BraggScatterer_sptr> scatterers;
  scatterers.reserve(tokens.size());
  std::transform(tokens.cbegin(), tokens.cend(), std::back_inserter(scatterers),
                 [this](const auto &token) { return getScatterer(token); });
  return scatterers;
}

/// Returns IsotropicAtomBraggScatterer for string with format "Element x y z
/// occupancy u_iso".
BraggScatterer_sptr IsotropicAtomBraggScattererParser::getScatterer(const std::string &singleScatterer) const {
  std::vector<std::string> tokens;
  boost::split(tokens, singleScatterer, boost::is_any_of(" "));

  if (tokens.size() < 4 || tokens.size() > 6) {
    throw std::invalid_argument("Could not parse scatterer string: " + singleScatterer);
  }

  std::vector<std::string> cleanScattererTokens = getCleanScattererTokens(tokens);
  std::vector<std::string> properties = {"Element", "Position", "Occupancy", "U"};

  ::Json::Value root;
  for (size_t i = 0; i < cleanScattererTokens.size(); ++i) {
    root[properties[i]] = cleanScattererTokens[i];
  }

  std::string initString = Mantid::JsonHelpers::jsonToString(root);

  return BraggScattererFactory::Instance().createScatterer("IsotropicAtomBraggScatterer", initString);
}

/// Converts tokens for getScatterer method so they can be processed by factory.
std::vector<std::string>
IsotropicAtomBraggScattererParser::getCleanScattererTokens(const std::vector<std::string> &tokens) const {
  std::vector<std::string> cleanTokens;

  // Element
  cleanTokens.emplace_back(tokens[0]);

  // X, Y, Z
  cleanTokens.emplace_back("[" + tokens[1] + "," + tokens[2] + "," + tokens[3] + "]");

  for (size_t i = 4; i < tokens.size(); ++i) {
    cleanTokens.emplace_back(tokens[i]);
  }

  return cleanTokens;
}

std::string getIsotropicAtomBraggScattererString(const BraggScatterer_sptr &scatterer) {
  IsotropicAtomBraggScatterer_sptr isotropicAtom = std::dynamic_pointer_cast<IsotropicAtomBraggScatterer>(scatterer);

  if (!isotropicAtom) {
    throw std::invalid_argument("Printing function can only process IsotropicAtomBraggScatterer.");
  }

  std::string rawPositionString = isotropicAtom->getProperty("Position");
  std::vector<std::string> positionComponents = getTokenizedPositionString(rawPositionString);

  std::stringstream outStream;
  outStream << isotropicAtom->getElement() << " " << positionComponents[0] << " " << positionComponents[1] << " "
            << positionComponents[2] << " " << isotropicAtom->getOccupancy() << " " << isotropicAtom->getU();

  return outStream.str();
}

} // namespace Mantid::Geometry
