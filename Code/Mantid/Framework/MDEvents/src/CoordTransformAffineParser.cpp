#include "MantidAPI/SingleValueParameterParser.h"
#include "MantidMDEvents/AffineMatrixParameterParser.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidMDEvents/CoordTransformAffine.h"
#include "MantidMDEvents/CoordTransformAffineParser.h"
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid {
namespace MDEvents {
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
  typedef Mantid::API::SingleValueParameterParser<Mantid::API::InDimParameter>
      InDimParameterParser;
  typedef Mantid::API::SingleValueParameterParser<Mantid::API::OutDimParameter>
      OutDimParameterParser;
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
  Poco::XML::NodeList *parameters =
      paramListElement->getElementsByTagName("Parameter");

  // Add input dimension parameter.
  InDimParameterParser inDimParser;
  Poco::XML::Element *parameter =
      dynamic_cast<Poco::XML::Element *>(parameters->item(0));
  Mantid::API::InDimParameter *inDim =
      inDimParser.createWithoutDelegation(parameter);

  // Add output dimension parameter.
  OutDimParameterParser outDimParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(1));
  Mantid::API::OutDimParameter *outDim =
      outDimParser.createWithoutDelegation(parameter);

  // Add affine matrix parameter.
  AffineMatrixParameterParser affineMatrixDimParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(2));
  AffineMatrixParameter *affineMatrix =
      affineMatrixDimParser.createParameter(parameter);

  // Generate the coordinate transform with the matrix and return.
  CoordTransformAffine *transform =
      new CoordTransformAffine(inDim->getValue(), outDim->getValue());
  transform->setMatrix(affineMatrix->getAffineMatrix());
  return transform;
}

//-----------------------------------------------------------------------------------------------
/**
Set the successor parser.
@param other : another parser to use if this one fails.
*/
void
CoordTransformAffineParser::setSuccessor(CoordTransformAffineParser *other) {
  m_successor = SuccessorType_sptr(other);
}

/// Destructor
CoordTransformAffineParser::~CoordTransformAffineParser() {}
}
}
