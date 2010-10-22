#include "MantidMDAlgorithms/InvalidParameterParser.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
    namespace MDAlgorithms
    {

        InvalidParameterParser::InvalidParameterParser()
        {

        }

        std::auto_ptr<IParameter> InvalidParameterParser::createParameter(Poco::XML::Element* parameterElement)
        {
            std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();
            return std::auto_ptr<IParameter>(parseInvalidParameter(sParameterValue));
        }


        InvalidParameter* InvalidParameterParser::parseInvalidParameter(std::string value)
        {
            return new InvalidParameter(value);
        }

        void InvalidParameterParser::setSuccessorParser(std::auto_ptr<ParameterParser> parser)
        {
            //Do nothing.
        }
    }


}