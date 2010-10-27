#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
    namespace MDAlgorithms
    {

        InvalidParameterParser::InvalidParameterParser()
        {

        }

        Mantid::API::ImplicitFunctionParameter* InvalidParameterParser::createParameter(Poco::XML::Element* parameterElement)
        {
            std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
            return parseInvalidParameter(sParameterValue);
        }


        InvalidParameter* InvalidParameterParser::parseInvalidParameter(std::string value)
        {
            return new InvalidParameter(value);
        }

        void InvalidParameterParser::setSuccessorParser(ImplicitFunctionParameterParser* parser)
        {
            //Do nothing. No sucessor allowed.
        }
    }


}