#include "MantidGeometry/Crystal/BraggScattererFactory.h"
#include "MantidKernel/LibraryManager.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

namespace Mantid {
namespace Geometry {

/**
 * Creates an initialized instance of the desired scatterer class
 *
 * This method tries to construct an instance of the class specified by the
 * "name"-parameter. If it is not found, an exception is thrown
 * (see DynamicFactory::create). Otherwise, the object is initialized.
 * If the second argument is not empty, it is expected to contain a semi-colon
 * separated list of "name=value"-pairs. These pairs need to be
 * valid input for assigning properties of the created scatterer. See the
 * example in the general class documentation.
 *
 * @param name :: Class name to construct.
 * @param properties :: Semi-colon separated "name=value"-pairs.
 * @return Initialized scatterer object.
 */
BraggScatterer_sptr BraggScattererFactoryImpl::createScatterer(
    const std::string &name, const std::string &properties) const {
  BraggScatterer_sptr scatterer = create(name);
  scatterer->initialize();

  if (!properties.empty()) {
    scatterer->setProperties(properties);
  }

  return scatterer;
}

/**
 * Returns a CompositeBraggScatterer with IsotropicAtomBraggScatterers
 *
 * The function expects a string in the following format:
 *
 *  Element x y z occupancy u_iso; Element x y z occupancy u_iso; ...
 *
 * It generates an IsotropicAtomBraggScatterer for each specified atom.
 *
 * @param scattererString :: String in the format specified above
 * @return CompositeBraggScatterer with specified scatterers
 */
CompositeBraggScatterer_sptr
BraggScattererFactoryImpl::createIsotropicScatterers(
    const std::string &scattererString) const {
  return getScatterers(scattererString);
}

CompositeBraggScatterer_sptr BraggScattererFactoryImpl::getScatterers(
    const std::string &scattererString) const {
  boost::char_separator<char> atomSep(";");
  boost::tokenizer<boost::char_separator<char> > tokens(scattererString,
                                                        atomSep);

  std::vector<BraggScatterer_sptr> scatterers;

  for (auto it = tokens.begin(); it != tokens.end(); ++it) {
    scatterers.push_back(getScatterer(boost::trim_copy(*it)));
  }

  return CompositeBraggScatterer::create(scatterers);
}

/// Returns IsotropicAtomBraggScatterer for string with format "Element x y z
/// occupancy u_iso".
BraggScatterer_sptr BraggScattererFactoryImpl::getScatterer(
    const std::string &singleScatterer) const {
  std::vector<std::string> tokens;
  boost::split(tokens, singleScatterer, boost::is_any_of(" "));

  if (tokens.size() < 4 || tokens.size() > 6) {
    throw std::invalid_argument("Could not parse scatterer string: " +
                                singleScatterer);
  }

  std::vector<std::string> cleanScattererTokens =
      getCleanScattererTokens(tokens);
  std::vector<std::string> properties =
      boost::assign::list_of("Element")("Position")("Occupancy")("U")
          .convert_to_container<std::vector<std::string> >();

  std::string initString;
  for (size_t i = 0; i < cleanScattererTokens.size(); ++i) {
    initString += properties[i] + "=" + cleanScattererTokens[i] + ";";
  }

  return createScatterer("IsotropicAtomBraggScatterer", initString);
}

/// Converts tokens for getScatterer method so they can be processed by factory.
std::vector<std::string> BraggScattererFactoryImpl::getCleanScattererTokens(
    const std::vector<std::string> &tokens) const {
  std::vector<std::string> cleanTokens;

  // Element
  cleanTokens.push_back(tokens[0]);

  // X, Y, Z
  cleanTokens.push_back("[" + tokens[1] + "," + tokens[2] + "," + tokens[3] +
                        "]");

  for (size_t i = 4; i < tokens.size(); ++i) {
    cleanTokens.push_back(tokens[i]);
  }

  return cleanTokens;
}

/// Private constructor.
BraggScattererFactoryImpl::BraggScattererFactoryImpl() {
  Kernel::LibraryManager::Instance();
}

} // namespace Geometry
} // namespace Mantid
