#include "MantidMDAlgorithms/CompositeFunctionParser.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        CompositeFunctionParser::CompositeFunctionParser() : ImplicitFunctionParser(new InvalidParameterParser)
        {
        }

        API::ImplicitFunctionBuilder* CompositeFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
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

        void CompositeFunctionParser::setSuccessorParser(ImplicitFunctionParser* parser)
        {
            this->m_successor = std::auto_ptr<ImplicitFunctionParser>(parser);
        }

        CompositeFunctionBuilder * CompositeFunctionParser::parseCompositeFunction(Poco::XML::Element* functionElement)
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

        CompositeFunctionParser::~CompositeFunctionParser()
        {
        }
    }
}