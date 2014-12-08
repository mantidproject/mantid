#include "MantidVatesSimpleGuiQtWidgets/GeometryParser.h"
#include "MantidVatesSimpleGuiQtWidgets/AxisInformation.h"

#include "MantidGeometry/MDGeometry/MDGeometryXMLDefinitions.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/DOMWriter.h>

#include <iostream>
#include <sstream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{

GeometryParser::GeometryParser(const char *xml)
{
  Poco::XML::DOMParser parser;
  this->pDoc = parser.parseString(Poco::XML::XMLString(xml));
}

AxisInformation *GeometryParser::getAxisInfo(const std::string dimension)
{
  AxisInformation *axis = new AxisInformation();

  Poco::AutoPtr<Poco::XML::NodeList> pNodes = this->pDoc->getElementsByTagName(dimension);
  Poco::XML::Node *pNode = pNodes->item(0)->childNodes()->item(0);
  Poco::XML::XMLString label = pNode->innerText();

  pNodes = this->pDoc->getElementsByTagName("Dimension");
  for (unsigned long int i = 0; i < pNodes->length(); ++i)
  {
    pNode = pNodes->item(i);
    Poco::XML::NamedNodeMap *aMap = pNode->attributes();
    Poco::XML::XMLString id = aMap->getNamedItem("ID")->getNodeValue();
    if (id == label)
    {
      break;
    }
  }

  Poco::AutoPtr<Poco::XML::NodeList> cNodes = pNode->childNodes();
  double min = -99.0;
  double max = 99.0;
  std::string title;
  // Using ID for now. Remove if we go back to using axis name
  title = label;
  for (unsigned long int j = 0; j < cNodes->length(); ++j)
  {
    Poco::XML::Node *cNode = cNodes->item(j);
    Poco::XML::XMLString elem = cNode->nodeName();
    // Keeping below around in case we go back to using axis name
    /*
        if (elem == Poco::XML::XMLString("Name"))
        {
        title = cNode->innerText();
        }
      */
    if (elem == Poco::XML::XMLString("LowerBounds"))
    {
      min = this->convertBounds(cNode->innerText());
    }
    if (elem == Poco::XML::XMLString("UpperBounds"))
    {
      max = this->convertBounds(cNode->innerText());
    }
  }

  axis->setTitle(title);
  axis->setMinimum(min);
  axis->setMaximum(max);

  return axis;
}

double GeometryParser::convertBounds(Poco::XML::XMLString val)
{
  double temp;
  std::stringstream number(val);
  number >> temp;
  return temp;
}

/**
 * This function takes a timestep value and places it within the geometry
 * XML held by this object.
 * @param time the value of the timestep
 * @return the XML geometry with the timestep value added
 */
std::string GeometryParser::addTDimValue(double time)
{
  std::string tDimLabel = Mantid::Geometry::MDGeometryXMLDefinitions::workspaceTDimensionElementName();
  Poco::AutoPtr<Poco::XML::NodeList> pNodes = this->pDoc->getElementsByTagName(tDimLabel);
  Poco::XML::Node *pNode = pNodes->item(0);
  std::ostringstream timeStr;
  timeStr << time;
  Poco::AutoPtr<Poco::XML::Element> valueElement = this->pDoc->createElement("Value");
  Poco::AutoPtr<Poco::XML::Text> valueText = this->pDoc->createTextNode(timeStr.str());
  valueElement->appendChild(valueText);
  pNode->appendChild(valueElement);

  std::stringstream xmlstream;
  Poco::XML::DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);
  return xmlstream.str();
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
