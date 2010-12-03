
#include "MantidMDAlgorithms/InvalidParameter.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        InvalidParameter::InvalidParameter()
        {
        }

        InvalidParameter::InvalidParameter(std::string value): m_value(value) 
        {
        }

        std::string InvalidParameter::getName() const
        {
            return parameterName();
        }

        std::string InvalidParameter::getValue() const
        {
            return m_value;
        }

        bool InvalidParameter::isValid() const
        {
            return false;
        }

        InvalidParameter* InvalidParameter::clone() const
        {
            return new InvalidParameter(m_value);
        }

        InvalidParameter::~InvalidParameter()
        {
        }

        std::string InvalidParameter::toXMLString() const
        {
            throw std::runtime_error("Invalid parameters cannot be represented in xml.");
        }
    }


}
