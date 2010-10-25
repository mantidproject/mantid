#include "MantidMDAlgorithms/NormalParameterParser.h"
#include <boost/algorithm/string.hpp>

namespace Mantid
{
    namespace MDAlgorithms
    {

        NormalParameterParser::NormalParameterParser()
        {

        }

        NormalParameterParser::~NormalParameterParser()
        {
        }

        IParameter* NormalParameterParser::createParameter(Poco::XML::Element* parameterElement)
        {
            if(NormalParameter::parameterName() != parameterElement->getChildElement("Type")->innerText())
            {
                return m_successor->createParameter(parameterElement);
            }
            else
            {
                std::string sParameterValue = parameterElement->getChildElement("Value")->innerText();

                return parseNormalParameter(sParameterValue);
            }
        }

        NormalParameter* NormalParameterParser::parseNormalParameter(std::string value)
        {
            std::vector<std::string> strs;
            boost::split(strs, value, boost::is_any_of(","));
            double nx, ny, nz;
            try
            {
                nx = atof(strs.at(0).c_str());
                ny = atof(strs.at(1).c_str());
                nz = atof(strs.at(2).c_str());
            }
            catch(std::exception& ex)
            {
                std::string message = std::string(ex.what()) + " Failed to parse NormalParameter value: " + value;
                throw std::invalid_argument(message.c_str());
            }
            return new NormalParameter(nx, ny, nz);
        }

        void NormalParameterParser::setSuccessorParser(ParameterParser* parameterParser)
        {
            m_successor = std::auto_ptr<ParameterParser>(parameterParser);
        }
    }


}