#include "MantidMDAlgorithms/PlaneFunctionParser.h"
#include "MantidMDAlgorithms/PlaneImplicitFunction.h"
namespace Mantid
{
    namespace MDAlgorithms
    {
        PlaneFunctionParser::PlaneFunctionParser(std::auto_ptr<ParameterParser> parameterParser) : FunctionParser(parameterParser)
        {
        }

        std::auto_ptr<IFunctionBuilder> PlaneFunctionParser::createFunctionBuilder(Poco::XML::Element* functionElement)
        {
            std::auto_ptr<IFunctionBuilder> functionBuilder;
            if("Function" != functionElement->localName())
            {
               std::string message = "This is not a function element: " + functionElement->localName(); 
               throw std::invalid_argument(message);
            }
            
            std::string type = functionElement->getChildElement("Type")->innerText();
            if(PlaneImplicitFunction::functionName() != type)
            {
                FunctionParser::checkSuccessorExists();
                functionBuilder = m_successor->createFunctionBuilder(functionElement);
            }
            else
            {
                functionBuilder = std::auto_ptr<IFunctionBuilder>(parsePlaneFunction(functionElement));
            }
            return functionBuilder;
        }

        void PlaneFunctionParser::setSuccessorParser(std::auto_ptr<FunctionParser> parser)
        {
            this->m_successor = parser;
        }

        PlaneFunctionBuilder * PlaneFunctionParser::parsePlaneFunction(Poco::XML::Element* functionElement)
        {
            using namespace Poco::XML;
            std::auto_ptr<PlaneFunctionBuilder> functionBuilder = std::auto_ptr<PlaneFunctionBuilder>(new PlaneFunctionBuilder);
            NodeList* parameterList = functionElement->getChildElement("ParameterList")->childNodes();
            
            for(int i = 0; i < parameterList->length(); i++)
            {
                Element* parameterElement = dynamic_cast<Element*>(parameterList->item(i));
                std::auto_ptr<IParameter> parameter = std::auto_ptr<IParameter>(this->parseParameter(parameterElement));
                if(NormalParameter::parameterName() == parameter->getName())
                { 
                    NormalParameter normal = NormalParameter(dynamic_cast<NormalParameter*>(parameter.get()));
                    functionBuilder->addNormalParameter(normal);
                }
                else if(OriginParameter::parameterName() == parameter->getName())
                {
                    OriginParameter origin = OriginParameter(dynamic_cast<OriginParameter*>(parameter.get()));
                    functionBuilder->addOriginParameter(origin);
                }
                else
                {
                    //Do nothing!
                }
                
            }

            return functionBuilder.release(); //convert to raw pointer and cancel smart management.
        }
    }
}