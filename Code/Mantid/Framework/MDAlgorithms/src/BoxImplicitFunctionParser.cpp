#include "MantidMDAlgorithms/BoxImplicitFunctionParser.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    DECLARE_IMPLICIT_FUNCTION_PARSER(BoxImplicitFunctionParser);

    BoxImplicitFunctionParser::BoxImplicitFunctionParser() : ImplicitFunctionParser(new InvalidParameterParser)
    {
    }

    API::ImplicitFunctionBuilder* BoxImplicitFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
    {
      API::ImplicitFunctionBuilder* functionBuilder;
      if("Function" != functionElement->localName())
      {
        std::string message = "This is not a function element: " + functionElement->localName(); 
        throw std::invalid_argument(message);
      }

      std::string type = functionElement->getChildElement("Type")->innerText();
      if(BoxImplicitFunction::functionName() != type) // If this parser does not match the xml, try the successor chain.
      {
        if(0 == m_successor.get())
        {
          std::string message = "There is no successor function parser here for type: " + type;
          throw std::runtime_error(message);
        }
        functionBuilder = m_successor->createFunctionBuilder(functionElement);
      }
      else // Parse the xml here.
      {
        functionBuilder = parseBoxFunction(functionElement);
      }
      return functionBuilder;
    }

    void BoxImplicitFunctionParser::setSuccessorParser(ImplicitFunctionParser* parser)
    {
      this->m_successor = std::auto_ptr<ImplicitFunctionParser>(parser);
    }

    BoxFunctionBuilder * BoxImplicitFunctionParser::parseBoxFunction(Poco::XML::Element* functionElement)
    {
      using namespace Poco::XML;
      BoxFunctionBuilder* functionBuilder = new BoxFunctionBuilder;
      NodeList* parameterList = functionElement->getChildElement("ParameterList")->childNodes();

      //Loop through the parameter list looking for known parameter, required, which can then be parsed.
      for(int i = 0; i < parameterList->length(); i++)
      {
        
        Element* parameterElement = dynamic_cast<Element*>(parameterList->item(i));

       std::string namee= parameterElement->localName();

        std::auto_ptr<API::ImplicitFunctionParameter> parameter = std::auto_ptr<API::ImplicitFunctionParameter>(this->parseParameter(parameterElement));
        if(WidthParameter::parameterName() == parameter->getName())
        { 
          WidthParameter* pCurr = dynamic_cast<WidthParameter*>(parameter.get());
          WidthParameter width(pCurr->getValue());
          functionBuilder->addWidthParameter(width);
        }
        else if(HeightParameter::parameterName() == parameter->getName())
        { 
          HeightParameter* pCurr = dynamic_cast<HeightParameter*>(parameter.get());
          HeightParameter height(pCurr->getValue());
          functionBuilder->addHeightParameter(height);
        }
        else if(DepthParameter::parameterName() == parameter->getName())
        { 
          DepthParameter* pCurr = dynamic_cast<DepthParameter*>(parameter.get());
          DepthParameter depth(pCurr->getValue());
          functionBuilder->addDepthParameter(depth);
        }
        else if(OriginParameter::parameterName() == parameter->getName())
        {
          OriginParameter*  pCurr = dynamic_cast<OriginParameter*>(parameter.get());
          OriginParameter origin(pCurr->getX(), pCurr->getY(), pCurr->getZ());
          functionBuilder->addOriginParameter(origin);
        }
        else
        {
          std::string message = "The parameter cannot be processed or is unrecognised: " + parameter->getName();
          message += ". The parameter cannot be processed or is unrecognised: " + (dynamic_cast<InvalidParameter*>(parameter.get()))->getValue();
          throw std::invalid_argument(message);
        }

      }

      return functionBuilder; //convert to raw pointer and cancel smart management.
    }

    BoxImplicitFunctionParser::~BoxImplicitFunctionParser()
    {
    }

    void BoxImplicitFunctionParser::setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
    {
      this->m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
    }
  }
}
