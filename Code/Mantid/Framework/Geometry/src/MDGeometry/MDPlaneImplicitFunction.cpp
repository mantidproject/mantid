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

MDPlaneImplicitFunction::MDPlaneImplicitFunction(const size_t nd,
                                                 const coord_t *normal,
                                                 const coord_t *point) :
  MDImplicitFunction()
{
  this->origin = new coord_t[nd];
  for( std::size_t i = 0; i < nd; i++)
  {
    this->origin[i] = point[i];
  }
  this->addPlane(MDPlane(nd, normal, point));
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

  // Normal Parameter
  AutoPtr<Element> normParameterElement = pDoc->createElement("Parameter");
  parameterListElement->appendChild(normParameterElement);
  AutoPtr<Element> normTypeElement = pDoc->createElement("Type");
  AutoPtr<Text> normText = pDoc->createTextNode("NormalParameter");
  normTypeElement->appendChild(normText);
  normParameterElement->appendChild(normTypeElement);
  AutoPtr<Element>normValueElement = pDoc->createElement("Value");
  const coord_t *norm = this->getPlane(0).getNormal();
  AutoPtr<Text> normValueText = pDoc->createTextNode(this->coordValue(norm));
  normValueElement->appendChild(normValueText);
  normParameterElement->appendChild(normValueElement);

  // Origin Parameter
  AutoPtr<Element> origParameterElement = pDoc->createElement("Parameter");
  parameterListElement->appendChild(origParameterElement);
  AutoPtr<Element> origTypeElement = pDoc->createElement("Type");
  AutoPtr<Text> origText = pDoc->createTextNode("OriginParameter");
  origTypeElement->appendChild(origText);
  origParameterElement->appendChild(origTypeElement);
  AutoPtr<Element>origValueElement = pDoc->createElement("Value");
  origParameterElement->appendChild(origValueElement);
  AutoPtr<Text> origValueText = pDoc->createTextNode(this->coordValue(this->origin));
  origValueElement->appendChild(origValueText);
  origParameterElement->appendChild(origValueElement);

  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString = \
      boost::str(boost::format(xmlstream.str().c_str()));
  return formattedXMLString;
}

std::string MDPlaneImplicitFunction::coordValue(const coord_t *arr) const
{
  std::ostringstream valueStream;
  std::size_t nd = this->getNumDims();
  for (std::size_t i = 0; i < nd - 1; i++)
  {
    valueStream << arr[i] << " ";
  }
  valueStream << arr[nd-1];
  return valueStream.str();
}

} // namespace Geometry
} // namespace Mantid
