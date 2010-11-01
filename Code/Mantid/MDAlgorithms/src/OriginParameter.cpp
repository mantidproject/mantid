#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/OriginParameter.h"

namespace Mantid
{
    namespace MDAlgorithms
    {

        OriginParameter::OriginParameter(double n1, double n2, double n3) 
        {
            m_origin.push_back(n1);
            m_origin.push_back(n2);
            m_origin.push_back(n3);
            m_isValid = true;
        }

        OriginParameter::OriginParameter()
        {
            this->m_isValid = false;
        }

        OriginParameter::OriginParameter(const OriginParameter& other)
        {
            this->m_isValid = other.m_isValid;
            this->m_origin = std::vector<double>(3);
            std::copy(other.m_origin.begin(), other.m_origin.end(), this->m_origin.begin());
        }

        OriginParameter& OriginParameter::operator=(const OriginParameter& other)
        {
            if(&other != this)
            {
                this->m_isValid = other.m_isValid;
                this->m_origin.clear();
                this->m_origin = std::vector<double>(3);
                std::copy(other.m_origin.begin(), other.m_origin.end(), this->m_origin.begin());
            }
            return *this;
        }

        std::string OriginParameter::getName() const
        {
            return parameterName();
        }

        bool OriginParameter::isValid() const
        {
            return m_isValid;
        }

        OriginParameter* OriginParameter::cloneImp() const
        {
            return new OriginParameter(m_origin.at(0), m_origin.at(1), m_origin.at(2));
        }

        std::auto_ptr<OriginParameter> OriginParameter::clone() const
        {
            return std::auto_ptr<OriginParameter>(cloneImp());
        }

        OriginParameter::~OriginParameter()
        {
        }

        double OriginParameter::getX() const
        {
            return m_origin.at(0);
        }

        double OriginParameter::getY() const
        {
            return m_origin.at(1);
        }

        double OriginParameter::getZ() const
        {
            return m_origin.at(2);
        }

        bool OriginParameter::operator==(const OriginParameter &other) const
        {
            return this->m_origin.at(0) == other.m_origin.at(0) && this->m_origin.at(1) == other.m_origin.at(1) && this->m_origin.at(2) == other.m_origin.at(2);
        }

        bool OriginParameter::operator!=(const OriginParameter &other) const
        {
            return !(*this == other);
        }

       std::string OriginParameter::toXMLString() const
        {
            std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_origin.at(0) %  m_origin.at(1) % m_origin.at(2));

            return this->parameterXMLTemplate(valueXMLtext);
        }
    }


}
