#include <Poco/DOM/Element.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/XML/XMLException.h>
#include <Poco/AutoPtr.h>
#include <Poco/NumberParser.h>
#include <boost/make_shared.hpp>

#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"

namespace Mantid {
namespace Geometry {

/** Create a dimension object from the provided XML string.
 *  @param dimensionXMLString The XML string
 *  @throw Poco::XML::SAXParseException If the provided string is not valid XML
 *  @return The created dimension.
 */
IMDDimension_sptr createDimension(const std::string &dimensionXMLString) {
  Poco::XML::DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc;
  try {
    pDoc = pParser.parseString(dimensionXMLString);

  } catch (Poco::XML::XMLException &ex) {
    // Transform into std::invalid_argument
    throw std::invalid_argument(
        std::string("Invalid string passed to createDimension: ") + ex.what());
  }
  return createDimension(*pDoc->documentElement());
}

/// Create a dimension from the provided XML element.
IMDDimension_sptr createDimension(const Poco::XML::Element &dimensionXML) {
  Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = dimensionXML.attributes();

  // First and only attribute is the dimension id.
  // Poco::XML::Node* dimensionId = attributes->item(0);
  const std::string id =
      dimensionXML.getAttribute("ID"); // dimensionId->innerText();
  if (id.empty()) {
    throw std::invalid_argument(
        "Invalid string passed to createDimension: No ID attribute");
  }

  Poco::XML::Element *nameElement = dimensionXML.getChildElement("Name");
  if (NULL == nameElement) {
    throw std::invalid_argument(
        "Invalid string passed to createDimension: No Name element");
  }
  const std::string name = nameElement->innerText();

  Poco::XML::Element *unitsElement = dimensionXML.getChildElement("Units");
  std::string units = "None";
  if (NULL != unitsElement) {
    // Set units if they exist.
    units = unitsElement->innerText();
  }

  Poco::XML::Element *upperBoundsElement =
      dimensionXML.getChildElement("UpperBounds");
  if (NULL == upperBoundsElement) {
    throw std::invalid_argument(
        "Invalid string passed to createDimension: No UpperBounds element");
  }
  Poco::XML::Element *lowerBoundsElement =
      dimensionXML.getChildElement("LowerBounds");
  if (NULL == lowerBoundsElement) {
    throw std::invalid_argument(
        "Invalid string passed to createDimension: No LowerBounds element");
  }

  double upperBounds, lowerBounds;
  try {
    upperBounds =
        Poco::NumberParser::parseFloat(upperBoundsElement->innerText());
    lowerBounds =
        Poco::NumberParser::parseFloat(lowerBoundsElement->innerText());
  } catch (Poco::SyntaxException &ex) {
    throw std::invalid_argument(
        std::string("Invalid string passed to createDimension: ") + ex.what());
  }

  Poco::XML::Element *numBinsElement =
      dimensionXML.getChildElement("NumberOfBins");
  if (NULL == numBinsElement) {
    throw std::invalid_argument(
        "Invalid string passed to createDimension: No NumberOfBins element");
  }
  unsigned int nBins;
  try {
    nBins = Poco::NumberParser::parseUnsigned(numBinsElement->innerText());
  } catch (Poco::SyntaxException &ex) {
    throw std::invalid_argument(
        std::string("Invalid string passed to createDimension: ") + ex.what());
  }

  Poco::XML::Element *integrationXML =
      dimensionXML.getChildElement("Integrated");
  if (NULL != integrationXML) {
    double upperLimit = atof(
        integrationXML->getChildElement("UpperLimit")->innerText().c_str());
    double lowerLimit = atof(
        integrationXML->getChildElement("LowerLimit")->innerText().c_str());

    // As it is not currently possible to set integration ranges on a
    // MDDimension or MDGeometryDescription, boundaries become integration
    // ranges.
    upperBounds = upperLimit;
    lowerBounds = lowerLimit;
  }

  return boost::make_shared<MDHistoDimension>(
      name, id, units, static_cast<coord_t>(lowerBounds),
      static_cast<coord_t>(upperBounds), nBins);
}

/** Create a dimension object from the provided XML string, overriding certain
 * attributes.
 *  @param dimensionXMLString The XML string from which to construct the
 * dimension object.
 *  @param nBins              The number of bins to set on the dimension object.
 *  @param min                The minimum extent to set on the dimension.
 *  @param max                The maximum extent to set on the dimension.
 *  @return The created dimension.
 */
IMDDimension_sptr createDimension(const std::string &dimensionXMLString,
                                  int nBins, coord_t min, coord_t max) {
  auto dimension = createDimension(dimensionXMLString);
  dimension->setRange(nBins, min, max);
  return dimension;
}

} // namespace
} // namespace
