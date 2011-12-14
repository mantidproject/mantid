#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/AutoPtr.h>
#include <sstream>

namespace Mantid
{
namespace Geometry
{

MDPlaneImplicitFunction::MDPlaneImplicitFunction() : MDImplicitFunction()
{
}

MDPlaneImplicitFunction::~MDPlaneImplicitFunction()
{
}

void MDPlaneImplicitFunction::addPlane(const MDPlane &plane)
{
  if (this->getNumPlanes() > 0)
  {
    throw std::runtime_error("Only one plane per MDPlaneImplicitFunction.");
  }
  else
  {
    MDImplicitFunction::addPlane(plane);
  }
}

std::string MDPlaneImplicitFunction::getName() const
{
  return std::string("PlaneImplicitFuction");
}

std::string MDPlaneImplicitFunction::toXMLString() const
{
  using namespace Poco::XML;
  AutoPtr<Document> pDoc = new Document;
  AutoPtr<Element> functionElement = pDoc->createElement("Function");
  pDoc->appendChild(functionElement);
  AutoPtr<Element> typeElement = pDoc->createElement("Type");
  AutoPtr<Text> typeText = pDoc->createTextNode(this->getName());
  typeElement->appendChild(typeText);
  functionElement->appendChild(typeElement);

  AutoPtr<Element> parameterListElement = pDoc->createElement("ParameterList");
  functionElement->appendChild(parameterListElement);

  AutoPtr<Element> valueElement = pDoc->createElement("Value");

  // Normal Parameter
  AutoPtr<Element> normParameterElement = pDoc->createElement("Parameter");
  parameterListElement->appendChild(normParameterElement);
  AutoPtr<Element> normTypeElement = pDoc->createElement("Type");
  AutoPtr<Text> normText = pDoc->createTextNode("NormalParameter");
  normTypeElement->appendChild(normText);
  normParameterElement->appendChild(normTypeElement);

  // Origin Parameter
  AutoPtr<Element> origParameterElement = pDoc->createElement("Parameter");
  parameterListElement->appendChild(origParameterElement);
  AutoPtr<Element> origTypeElement = pDoc->createElement("Type");
  AutoPtr<Text> origText = pDoc->createTextNode("OriginParameter");
  origTypeElement->appendChild(origText);
  origParameterElement->appendChild(origTypeElement);

  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString = \
      boost::str(boost::format(xmlstream.str().c_str()));
  return formattedXMLString;
}

} // namespace Geometry
} // namespace Mantid
