#include "MantidMDAlgorithms/OriginParameterParser.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
    namespace MDAlgorithms
    {

        OriginParameterParser::OriginParameterParser()
        {

        }

        API::ImplicitFunctionParameter* OriginParameterParser::createParameter(Poco::XML::Element* parameterElement)
        {
            std::string sParameterType = parameterElement->getChildElement("Type")->innerText();
            if(OriginParameter::parameterName() != sParameterType)
            {
                return m_successor->createParameter(parameterElement);
            }
            else
            {

                std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
                return parseOriginParameter(sParameterValue);
            }
        }


        OriginParameter* OriginParameterParser::parseOriginParameter(std::string value)
        {
            std::vector<std::string> strs;
            boost::split(strs, value, boost::is_any_of(","));
            double ox, oy, oz;
            try
            {

                ox = atof(strs.at(0).c_str());
                oy = atof(strs.at(1).c_str());
                oz = atof(strs.at(2).c_str());
            }
            catch(std::exception& ex)
            {
                std::string message = std::string(ex.what()) + " Failed to parse OriginParameter value: " + value;
                throw std::invalid_argument(message.c_str());
            }
            return new OriginParameter(ox, oy, oz);
        }

        void OriginParameterParser::setSuccessorParser(ImplicitFunctionParameterParser* parameterParser)
        {
            m_successor = std::auto_ptr<ImplicitFunctionParameterParser>(parameterParser);
        }
    }


}