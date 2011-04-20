#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "MantidMDAlgorithms/PlaneImplicitFunctionParser.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    DECLARE_IMPLICIT_FUNCTION_PARSER(PlaneImplicitFunctionParser);

    PlaneImplicitFunctionParser::PlaneImplicitFunctionParser() : ImplicitFunctionParser(new InvalidParameterParser)
    {
    }

    API::ImplicitFunctionBuilder* PlaneImplicitFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      API::ImplicitFunctionBuilder* functionBuilder;
      if("Function" != functionElement->localName())
      {
        std::string message = "This is not a function element: " + functionElement->localName(); 
        throw std::invalid_argument(message);
      }

      std::string type = functionElement->getChildElement("Type")->innerText();
      if(PlaneImplicitFunction::functionName() != type)
      {
        if(0 == m_successor.get())
        {
          std::string message = "There is no successor function parser here for type: " + type;
          throw std::runtime_error(message);
        }
        functionBuilder = m_successor->createFunctionBuilder(functionElement);
      }
      else
      {
        functionBuilder = parsePlaneFunction(functionElement);
      }
      return functionBuilder;
    }

    void PlaneImplicitFunctionParser::setSuccessorParser(ImplicitFunctionParser* parser)
    {
      ImplicitFunctionParser::SuccessorType temp(parser);
      this->m_successor.swap(temp);
    }

    PlaneFunctionBuilder * PlaneImplicitFunctionParser::parsePlaneFunction(Poco::XML::Element* functionElement)
    {
      using namespace Poco::XML;
      boost::interprocess::unique_ptr<PlaneFunctionBuilder, Mantid::API::DeleterPolicy<PlaneFunctionBuilder> > functionBuilder(new PlaneFunctionBuilder);
      NodeList* parameterList = functionElement->getChildElement("ParameterList")->childNodes();

      //Loop through all parameters and attempt to identify those that are known to belong to this implicit function type.
      for(unsigned int i = 0; i < parameterList->length(); i++)
      {
        Element* parameterElement = dynamic_cast<Element*>(parameterList->item(i));
        boost::scoped_ptr<API::ImplicitFunctionParameter> parameter(this->parseParameter(parameterElement));
        if(NormalParameter::parameterName() == parameter->getName())
        { 
          NormalParameter* pCurr = dynamic_cast<NormalParameter*>(parameter.get());
          NormalParameter normal(pCurr->getX(), pCurr->getY(), pCurr->getZ());
          functionBuilder->addNormalParameter(normal);
        }
        else if(OriginParameter::parameterName() == parameter->getName())
        {
          OriginParameter*  pCurr = dynamic_cast<OriginParameter*>(parameter.get());
          OriginParameter origin(pCurr->getX(), pCurr->getY(), pCurr->getZ());
          functionBuilder->addOriginParameter(origin);
        }
        else if(WidthParameter::parameterName() == parameter->getName())
        {
          WidthParameter*  pCurr = dynamic_cast<WidthParameter*>(parameter.get());
          WidthParameter width(pCurr->getValue());
          functionBuilder->addWidthParameter(width);
        }
        else
        {
          std::string message = "The parameter cannot be processed or is unrecognised: " + parameter->getName();
          message += ". The parameter cannot be processed or is unrecognised: " + (dynamic_cast<InvalidParameter*>(parameter.get()))->getValue();
          throw std::invalid_argument(message);
        }

      }

      return functionBuilder.release(); //convert to raw pointer and cancel smart management.
    }

    PlaneImplicitFunctionParser::~PlaneImplicitFunctionParser()
    {
    }

    void PlaneImplicitFunctionParser::setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      Mantid::API::ImplicitFunctionParameterParser::SuccessorType temp(parser);
      this->m_paramParserRoot.swap(temp);
    }
  }
}
