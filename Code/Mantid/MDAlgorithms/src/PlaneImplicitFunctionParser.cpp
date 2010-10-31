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
                ImplicitFunctionParser::checkSuccessorExists();
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
            this->m_successor = std::auto_ptr<ImplicitFunctionParser>(parser);
        }

        PlaneFunctionBuilder * PlaneImplicitFunctionParser::parsePlaneFunction(Poco::XML::Element* functionElement)
        {
            using namespace Poco::XML;
            std::auto_ptr<PlaneFunctionBuilder> functionBuilder = std::auto_ptr<PlaneFunctionBuilder>(new PlaneFunctionBuilder);
            NodeList* parameterList = functionElement->getChildElement("ParameterList")->childNodes();
            
            for(int i = 0; i < parameterList->length(); i++)
            {
                Element* parameterElement = dynamic_cast<Element*>(parameterList->item(i));
                std::auto_ptr<API::ImplicitFunctionParameter> parameter = std::auto_ptr<API::ImplicitFunctionParameter>(this->parseParameter(parameterElement));
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
                else
                {
                    //Do nothing!
                }
                
            }

            return functionBuilder.release(); //convert to raw pointer and cancel smart management.
        }

        PlaneImplicitFunctionParser::~PlaneImplicitFunctionParser()
        {
        }

        void PlaneImplicitFunctionParser::setParameterParser(Mantid::API::ImplicitFunctionParameterParser* parser)
        {
          this->m_paramParserRoot = std::auto_ptr<Mantid::API::ImplicitFunctionParameterParser>(parser);
        }
    }
}
