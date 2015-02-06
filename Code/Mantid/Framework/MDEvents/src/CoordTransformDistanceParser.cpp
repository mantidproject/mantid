#include "MantidMDEvents/CoordTransformDistanceParser.h"
#include "MantidMDEvents/CoordTransformDistance.h"
#include "MantidAPI/VectorParameterParser.h"
#include "MantidAPI/SingleValueParameterParser.h"

namespace Mantid {
namespace MDEvents {
/// Constructor
CoordTransformDistanceParser::CoordTransformDistanceParser() {}

//-----------------------------------------------------------------------------------------------
/*
Create the transform object.
@param coordTransElement : xml coordinate transform element
@return a fully constructed coordinate transform object.
*/
Mantid::API::CoordTransform *CoordTransformDistanceParser::createTransform(
    Poco::XML::Element *coordTransElement) const {
  // Typdef the parameter parsers required.
  typedef Mantid::API::SingleValueParameterParser<Mantid::API::InDimParameter>
      InDimParameterParser;
  typedef Mantid::API::SingleValueParameterParser<Mantid::API::OutDimParameter>
      OutDimParameterParser;
  typedef Mantid::API::VectorParameterParser<CoordCenterVectorParam>
      CoordCenterParser;
  typedef Mantid::API::VectorParameterParser<DimensionsUsedVectorParam>
      DimsUsedParser;

  using namespace Poco::XML;
  if ("CoordTransform" != coordTransElement->localName()) {
    std::string message = "This is not a coordinate transform element: " +
                          coordTransElement->localName();
    throw std::invalid_argument(message);
  }
  if ("CoordTransformDistance" !=
      coordTransElement->getChildElement("Type")->innerText()) {
    // Delegate
    if (!m_successor) {
      throw std::runtime_error(
          "CoordTransformDistanceParser has no successor parser.");
    }
    return m_successor->createTransform(coordTransElement);
  }

  Element *paramListElement =
      coordTransElement->getChildElement("ParameterList");
  Poco::AutoPtr<Poco::XML::NodeList> parameters =
      paramListElement->getElementsByTagName("Parameter");

  // Parse the in dimension parameter.
  InDimParameterParser inDimParamParser;
  Poco::XML::Element *parameter =
      dynamic_cast<Poco::XML::Element *>(parameters->item(0));
  Mantid::API::InDimParameter *inDimParameter =
      inDimParamParser.createWithoutDelegation(parameter);

  // Parse the out dimension parameter.
  OutDimParameterParser outDimParamParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(1));
  Mantid::API::OutDimParameter *outDimParameter =
      outDimParamParser.createWithoutDelegation(parameter);
  UNUSED_ARG(outDimParameter); // not actually used as an input.

  // Parse the coordinate centre parameter.
  CoordCenterParser coordCenterParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(2));
  Mantid::MDEvents::CoordCenterVectorParam *coordCenterParam =
      coordCenterParser.createWithoutDelegation(parameter);

  // Parse the dimensions used parameter.
  DimsUsedParser dimsUsedParser;
  parameter = dynamic_cast<Poco::XML::Element *>(parameters->item(3));
  Mantid::MDEvents::DimensionsUsedVectorParam *dimsUsedVecParm =
      dimsUsedParser.createWithoutDelegation(parameter);

  ////Generate the coordinate transform and return
  CoordTransformDistance *transform = new CoordTransformDistance(
      inDimParameter->getValue(), coordCenterParam->getPointerToStart(),
      dimsUsedVecParm->getPointerToStart());
  return transform;
}

/// Destructor
CoordTransformDistanceParser::~CoordTransformDistanceParser() {}
}
}
