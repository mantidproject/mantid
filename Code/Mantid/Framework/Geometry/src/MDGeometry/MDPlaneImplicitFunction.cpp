#include "MantidGeometry/MDGeometry/MDPlaneImplicitFunction.h"

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/Text.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/AutoPtr.h>
#include <limits>
#include <sstream>

namespace Mantid {
namespace Geometry {

MDPlaneImplicitFunction::MDPlaneImplicitFunction() : MDImplicitFunction() {}

/**
 * This parameter constructor is used for when the origin of the implicit
 * plane is needed in the future. The coordinate arrays MUST be the same
 * length and match the specified number of dimensions.
 * @param nd the number of dimensions for the implicit plane
 * @param normal array of coordinates for the plane normal
 * @param point array of coorindates for the plane origin
 */
MDPlaneImplicitFunction::MDPlaneImplicitFunction(const size_t nd,
                                                 const float *normal,
                                                 const float *point)
    : MDImplicitFunction(), origin(nd) {
  for (std::size_t i = 0; i < nd; i++) {
    this->origin[i] = static_cast<coord_t>(point[i]);
  }
  this->addPlane(MDPlane(nd, normal, point));
}

/**
 * This parameter constructor is used for when the origin of the implicit
 * plane is needed in the future. The coordinate arrays MUST be the same
 * length and match the specified number of dimensions.
 * @param nd the number of dimensions for the implicit plane
 * @param normal array of coordinates for the plane normal
 * @param point array of coorindates for the plane origin
 */
MDPlaneImplicitFunction::MDPlaneImplicitFunction(const size_t nd,
                                                 const double *normal,
                                                 const double *point)
    : MDImplicitFunction(), origin(nd) {
  for (std::size_t i = 0; i < nd; i++) {
    this->origin[i] = static_cast<coord_t>(point[i]);
  }
  this->addPlane(MDPlane(nd, normal, point));
}

MDPlaneImplicitFunction::~MDPlaneImplicitFunction() {}

/**
 * This function overrides the inherited one in order to make sure that
 * only one plane is set on the implicit function.
 * @param plane the object containing the information for the implicit plane
 */
void MDPlaneImplicitFunction::addPlane(const MDPlane &plane) {
  if (this->getNumPlanes() > 0) {
    throw std::runtime_error("Only one plane per MDPlaneImplicitFunction.");
  } else {
    MDImplicitFunction::addPlane(plane);
    this->checkOrigin();
  }
}

std::string MDPlaneImplicitFunction::getName() const {
  return std::string("PlaneImplicitFuction");
}

std::string MDPlaneImplicitFunction::toXMLString() const {
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
  AutoPtr<Element> normValueElement = pDoc->createElement("Value");
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
  AutoPtr<Element> origValueElement = pDoc->createElement("Value");
  origParameterElement->appendChild(origValueElement);
  AutoPtr<Text> origValueText =
      pDoc->createTextNode(this->coordValue(this->origin.data()));
  origValueElement->appendChild(origValueText);
  origParameterElement->appendChild(origValueElement);

  std::stringstream xmlstream;
  DOMWriter writer;
  writer.writeNode(xmlstream, pDoc);

  std::string formattedXMLString =
      boost::str(boost::format(xmlstream.str().c_str()));
  return formattedXMLString;
}

/**
 * This is a helper function for converting a list of coordinate values into
 * a space separated string for the XML definition.
 * @param arr the array of coordinates to convert
 * @return the resulting values in a space separated string
 */
std::string MDPlaneImplicitFunction::coordValue(const coord_t *arr) const {
  std::ostringstream valueStream;
  std::size_t nd = this->getNumDims();
  for (std::size_t i = 0; i < nd - 1; i++) {
    valueStream << arr[i] << " ";
  }
  valueStream << arr[nd - 1];
  return valueStream.str();
}

void MDPlaneImplicitFunction::checkOrigin() {
  if (origin.empty()) {
    origin.resize(getNumDims(), std::numeric_limits<coord_t>::quiet_NaN());
  }
}

} // namespace Geometry
} // namespace Mantid
