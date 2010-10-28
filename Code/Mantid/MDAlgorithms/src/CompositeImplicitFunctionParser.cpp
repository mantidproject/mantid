#include "MantidMDAlgorithms/CompositeImplicitFunctionParser.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"
#include "MantidAPI/ImplicitFunctionParserFactory.h"
#include "MantidAPI/ImplicitFunctionParameterParserFactory.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        DECLARE_IMPLICIT_FUNCTION_PARSER(CompositeImplicitFunctionParser);

        CompositeImplicitFunctionParser::CompositeImplicitFunctionParser() : ImplicitFunctionParser(new InvalidParameterParser)
        {
        }

        API::ImplicitFunctionBuilder* CompositeImplicitFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
        {
            Mantid::API::ImplicitFunctionBuilder* functionBuilder;
            if("Function" != functionElement->localName())
            {
               std::string message = "This is not a function element: " + functionElement->localName(); 
               throw std::invalid_argument(message);
            }
            
            std::string type = functionElement->getChildElement("Type")->innerText();
            if(CompositeImplicitFunction::functionName() != type)
            {
                ImplicitFunctionParser::checkSuccessorExists(); 
                functionBuilder = m_successor->createFunctionBuilder(functionElement);
            }
            else
            {
                functionBuilder = parseCompositeFunction(functionElement);
            }
            return functionBuilder;
        }

        void CompositeImplicitFunctionParser::setSuccessorParser(ImplicitFunctionParser* parser)
        {
            this->m_successor = std::auto_ptr<ImplicitFunctionParser>(parser);
        }

        CompositeFunctionBuilder * CompositeImplicitFunctionParser::parseCompositeFunction(Poco::XML::Element* functionElement)
        {
            using namespace Poco::XML;
            ImplicitFunctionParser::checkSuccessorExists(); 
            std::auto_ptr<CompositeFunctionBuilder> functionBuilder = std::auto_ptr<CompositeFunctionBuilder>(new CompositeFunctionBuilder);
            NodeList* childFunctionElementList = functionElement->childNodes();
           
            for(int i = 0; i < childFunctionElementList->length(); i++)
            {
                Element* childFunctionElement = dynamic_cast<Element*>(childFunctionElementList->item(i));
                std::string typeName = childFunctionElement->localName();
                if("Function" == typeName)
                {
                    Mantid::API::ImplicitFunctionBuilder* childFunctionBuilder = m_successor->createFunctionBuilder(childFunctionElement);
                    functionBuilder->addFunctionBuilder(childFunctionBuilder);
                }
            }
            
            return functionBuilder.release(); 
        }

        CompositeImplicitFunctionParser::~CompositeImplicitFunctionParser()
        {
        }

        void CompositeImplicitFunctionParser::setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
        {
          this->m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
        }
    }
}