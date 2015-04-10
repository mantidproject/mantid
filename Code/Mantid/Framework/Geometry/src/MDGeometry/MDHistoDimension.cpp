#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <Poco/DOM/Attr.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>

namespace Mantid {
namespace Geometry {

std::string MDHistoDimension::toXMLString() const {
  using namespace Poco::XML;

  // Create the root element for this fragment.
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> pDimensionElement = pDoc->createElement("Dimension");
  pDoc->appendChild(pDimensionElement);

  // Set the id.
  AutoPtr<Attr> idAttribute = pDoc->createAttribute("ID");
  idAttribute->setNodeValue(this->getDimensionId());
  pDimensionElement->setAttributeNode(idAttribute);

  // Set the name.
  AutoPtr<Element> nameElement = pDoc->createElement("Name");
  AutoPtr<Text> nameText = pDoc->createTextNode(this->getName());
  nameElement->appendChild(nameText);
  pDimensionElement->appendChild(nameElement);

  // Set the units.
  AutoPtr<Element> unitsElement = pDoc->createElement("Units");
  AutoPtr<Text> unitsText = pDoc->createTextNode(this->getUnits());
  unitsElement->appendChild(unitsText);
  pDimensionElement->appendChild(unitsElement);

  // Set the upper bounds
  AutoPtr<Element> upperBoundsElement = pDoc->createElement("UpperBounds");
  AutoPtr<Text> upperBoundsText = pDoc->createTextNode(
      boost::str(boost::format("%.4f") % this->getMaximum()));
  upperBoundsElement->appendChild(upperBoundsText);
  pDimensionElement->appendChild(upperBoundsElement);

  // Set the lower bounds
  AutoPtr<Element> lowerBoundsElement = pDoc->createElement("LowerBounds");
  AutoPtr<Text> lowerBoundsText = pDoc->createTextNode(
      boost::str(boost::format("%.4f") % this->getMinimum()));
  lowerBoundsElement->appendChild(lowerBoundsText);
  pDimensionElement->appendChild(lowerBoundsElement);

  // Set the number of bins
  AutoPtr<Element> numberOfBinsElement = pDoc->createElement("NumberOfBins");
  AutoPtr<Text> numberOfBinsText = pDoc->createTextNode(
      boost::str(boost::format("%.4d") % this->getNBins()));
  numberOfBinsElement->appendChild(numberOfBinsText);
  pDimensionElement->appendChild(numberOfBinsElement);

  // Provide upper and lower limits for integrated dimensions.
  if (this->getIsIntegrated()) {
    AutoPtr<Element> integratedElement = pDoc->createElement("Integrated");
    // Set the upper limit
    AutoPtr<Element> upperLimitElement = pDoc->createElement("UpperLimit");
    AutoPtr<Text> upperLimitText = pDoc->createTextNode(boost::str(
        boost::format("%.4f") % this->getMaximum())); // Dimension does not yet
                                                      // provide integration
                                                      // ranges.
    upperLimitElement->appendChild(upperLimitText);
    integratedElement->appendChild(upperLimitElement);

    // Set the lower limit
    AutoPtr<Element> lowerLimitElement = pDoc->createElement("LowerLimit");
    AutoPtr<Text> lowerLimitText = pDoc->createTextNode(boost::str(
        boost::format("%.4f") % this->getMinimum())); // Dimension does not yet
                                                      // provide integration
                                                      // ranges.
    lowerLimitElement->appendChild(lowerLimitText);
    integratedElement->appendChild(lowerLimitElement);

    pDimensionElement->appendChild(integratedElement);
  }

  // Create a string representation of the DOM tree.
  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  return xmlstream.str().c_str();
}
}
}