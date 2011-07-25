#include "MantidMDEvents/CoordTransformParser.h"
#include "MantidMDEvents/CoordTransform.h"
#include "MantidMDEvents/AffineMatrixParameterParser.h"
#include "MantidAPI/SingleValueParameterParser.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>

namespace Mantid
{
  namespace MDEvents
  {
    CoordTransformParser::CoordTransformParser()
    {
    }

    CoordTransform* CoordTransformParser::createTransform(Poco::XML::Element* coordTransElement) const
    {
      typedef Mantid::API::SingleValueParameterParser<InDimParameter> InDimParameterParser;
      typedef Mantid::API::SingleValueParameterParser<OutDimParameter> OutDimParameterParser;
      using namespace Poco::XML;
      if("CoordTransform" != coordTransElement->localName())
      {
        std::string message = "This is not a coordinate transform element: " + coordTransElement->localName(); 
        throw std::invalid_argument(message);
      }
      if("CoordTransform" != coordTransElement->getChildElement("Type")->innerText())
      {
        //Delegate
        if(!m_successor)
        {
          throw std::runtime_error("CoordTransformParser has no successor parser.");
        }
        return m_successor->createTransform(coordTransElement);
      }

      Element* paramListElement = coordTransElement->getChildElement("ParameterList");
      if(!paramListElement)
      {
        throw std::runtime_error("No ParameterList element.");
      }

      Poco::XML::NodeList* parameters = paramListElement->getElementsByTagName("Parameter");

      InDimParameterParser inDimParser;
      Poco::XML::Element* parameter = dynamic_cast<Poco::XML::Element*>(parameters->item(0));
      InDimParameter* inDim = inDimParser.createWithoutDelegation(parameter);
      
      OutDimParameterParser outDimParser;
      parameter = dynamic_cast<Poco::XML::Element*>(parameters->item(1));
      OutDimParameter* outDim = outDimParser.createWithoutDelegation(parameter);
      
      AffineMatrixParameterParser affineMatrixDimParser;
      parameter = dynamic_cast<Poco::XML::Element*>(parameters->item(2));
      AffineMatrixParameter* affineMatrix = affineMatrixDimParser.createParameter(parameter);

      CoordTransform* transform = new CoordTransform(inDim->getValue(), outDim->getValue());
      transform->setMatrix(affineMatrix->getAffineMatrix());
      return transform;
    }

    void CoordTransformParser::setSuccessor(CoordTransformParser* other)
    {
      m_successor = SuccessorType_sptr(other);
    }

    CoordTransformParser::~CoordTransformParser()
    {
    }

  }
}