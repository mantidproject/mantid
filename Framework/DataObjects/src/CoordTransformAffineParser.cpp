#include "MantidDataObjects/CoordTransformAffineParser.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/SingleValueParameterParser.h"
#include "MantidDataObjects/AffineMatrixParameterParser.h"
#include "MantidDataObjects/CoordTransformAffine.h"

#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>

namespace Mantid {
namespace DataObjects {
/// Constructor
CoordTransformAffineParser::CoordTransformAffineParser() {}

//-----------------------------------------------------------------------------------------------
/**
Create the transform object.
@param coordTransElement : xml coordinate transform element
@return a fully constructed coordinate transform object.
*/
Mantid::API::CoordTransform *CoordTransformAffineParser::createTransform(
    Poco::XML::Element *coordTransElement) const {
  using InDimParameterParser =
      Mantid::API::SingleValueParameterParser<Mantid::API::InDimParameter>;
  using OutDimParameterParser =
      Mantid::API::SingleValueParameterParser<Mantid::API::OutDimParameter>;
  using namespace Poco::XML;
  if ("CoordTransform" != coordTransElement->localName()) {
    std::string message = "This is not a coordinate transform element: " +
                          coordTransElement->localName();
    throw std::invalid_argument(message);
  }
  if ("CoordTransformAffine" !=
      coordTransElement->getChildElement("Type")->innerText()) {
    // Delegate
    if (!m_successor) {
      throw std::runtime_error(
          "CoordTransformAffineParser has no successor parser.");
    }
    return m_successor->createTransform(coordTransElement);
  }

  Element *paramListElement =
      coordTransElement->getChildElement("ParameterList");
  Poco::AutoPtr<Poco::XML::NodeList> parameters =
      paramListElement->getElementsByTagName("Parameter");

  // Add input dimension parameter.
  InDimParameterParser inDimParser;
  Poco::XML::Element *parameter =
      dynamic_cast<Poco::XML::Element *>(parameters->item(0));
  boost::shared_ptr<Mantid::API::InDimParameter> inDim(
      inDimParser.createWithoutDelegation(parameter));

  // Add output dimension parameter.
  OutDimParameterParser outDimParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(1));
  boost::shared_ptr<Mantid::API::OutDimParameter> outDim(
      outDimParser.createWithoutDelegation(parameter));

  // Add affine matrix parameter.
  AffineMatrixParameterParser affineMatrixDimParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(2));
  boost::shared_ptr<AffineMatrixParameter> affineMatrix(
      affineMatrixDimParser.createParameter(parameter));

  // Generate the coordinate transform with the matrix and return.
  auto transform =
      new CoordTransformAffine(inDim->getValue(), outDim->getValue());
  transform->setMatrix(affineMatrix->getAffineMatrix());
  return transform;
}

//-----------------------------------------------------------------------------------------------
/**
Set the successor parser.
@param other : another parser to use if this one fails.
*/
void CoordTransformAffineParser::setSuccessor(
    CoordTransformAffineParser *other) {
  m_successor = SuccessorType_sptr(other);
}
} // namespace DataObjects
} // namespace Mantid
