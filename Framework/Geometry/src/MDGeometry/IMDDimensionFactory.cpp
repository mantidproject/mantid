// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/IMDDimensionFactory.h"
#include "MantidGeometry/MDGeometry/MDFrameFactory.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <Poco/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/NumberParser.h>
#include <Poco/XML/XMLException.h>
#include <memory>

namespace Mantid::Geometry {

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
    throw std::invalid_argument(std::string("Invalid string passed to createDimension: ") + ex.what());
  }
  return createDimension(*pDoc->documentElement());
}

/// Create a dimension from the provided XML element.
IMDDimension_sptr createDimension(const Poco::XML::Element &dimensionXML) {
  Poco::AutoPtr<Poco::XML::NamedNodeMap> attributes = dimensionXML.attributes();

  // First and only attribute is the dimension id.
  // Poco::XML::Node* dimensionId = attributes->item(0);
  const std::string id = dimensionXML.getAttribute("ID"); // dimensionId->innerText();
  if (id.empty()) {
    throw std::invalid_argument("Invalid string passed to createDimension: No ID attribute");
  }

  Poco::XML::Element *nameElement = dimensionXML.getChildElement("Name");
  if (nullptr == nameElement) {
    throw std::invalid_argument("Invalid string passed to createDimension: No Name element");
  }
  const std::string name = nameElement->innerText();

  Poco::XML::Element *unitsElement = dimensionXML.getChildElement("Units");
  std::string units = "None";
  if (nullptr != unitsElement) {
    // Set units if they exist.
    units = unitsElement->innerText();
  }

  Poco::XML::Element *frameElement = dimensionXML.getChildElement("Frame");
  std::string frame = "Unknown frame";
  if (nullptr != frameElement) {
    // Set the frame if it exists
    frame = frameElement->innerText();
  }

  Poco::XML::Element *upperBoundsElement = dimensionXML.getChildElement("UpperBounds");
  if (nullptr == upperBoundsElement) {
    throw std::invalid_argument("Invalid string passed to createDimension: No UpperBounds element");
  }
  Poco::XML::Element *lowerBoundsElement = dimensionXML.getChildElement("LowerBounds");
  if (nullptr == lowerBoundsElement) {
    throw std::invalid_argument("Invalid string passed to createDimension: No LowerBounds element");
  }

  double upperBounds, lowerBounds;
  try {
    upperBounds = Poco::NumberParser::parseFloat(upperBoundsElement->innerText());
    lowerBounds = Poco::NumberParser::parseFloat(lowerBoundsElement->innerText());
  } catch (Poco::SyntaxException &ex) {
    throw std::invalid_argument(std::string("Invalid string passed to createDimension: ") + ex.what());
  }

  Poco::XML::Element *numBinsElement = dimensionXML.getChildElement("NumberOfBins");
  if (nullptr == numBinsElement) {
    throw std::invalid_argument("Invalid string passed to createDimension: No NumberOfBins element");
  }
  unsigned int nBins;
  try {
    nBins = Poco::NumberParser::parseUnsigned(numBinsElement->innerText());
  } catch (Poco::SyntaxException &ex) {
    throw std::invalid_argument(std::string("Invalid string passed to createDimension: ") + ex.what());
  }

  Poco::XML::Element *integrationXML = dimensionXML.getChildElement("Integrated");
  if (nullptr != integrationXML) {
    double upperLimit = std::stod(integrationXML->getChildElement("UpperLimit")->innerText());
    double lowerLimit = std::stod(integrationXML->getChildElement("LowerLimit")->innerText());

    // As it is not currently possible to set integration ranges on a
    // MDDimension or MDGeometryDescription, boundaries become integration
    // ranges.
    upperBounds = upperLimit;
    lowerBounds = lowerLimit;
  }

  // Select the unit.
  MDFrame_const_uptr mdframe = makeMDFrameFactoryChain()->create(MDFrameArgument(frame, units));

  return std::make_shared<MDHistoDimension>(name, id, *mdframe, static_cast<coord_t>(lowerBounds),
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
IMDDimension_sptr createDimension(const std::string &dimensionXMLString, int nBins, coord_t min, coord_t max) {
  auto dimension = createDimension(dimensionXMLString);
  dimension->setRange(nBins, min, max);
  return dimension;
}

} // namespace Mantid::Geometry
