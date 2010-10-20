
#include "MantidMDAlgorithms/NormalParameter.h"

namespace Mantid
{
	namespace MDAlgorithms
	{
		NormalParameter::NormalParameter(double n1, double n2, double n3)
		{
			m_normal.push_back(n1);
			m_normal.push_back(n2);
			m_normal.push_back(n3);
		}

		NormalParameter::NormalParameter(NormalParameter& other)
		{
			this->m_normal = std::vector<double>(3);
			std::copy(other.m_normal.begin(), other.m_normal.end(), this->m_normal.begin());
		}

		NormalParameter NormalParameter::reflect()
		{
			return NormalParameter(-m_normal.at(0), -m_normal.at(1), -m_normal.at(2));
		}
		
		
		std::string NormalParameter::getName() const
		{
			return parameterName();
		}

		bool NormalParameter::isValid() const
		{
			return true;
		}

		NormalParameter* NormalParameter::cloneImp() const
		{
			return new NormalParameter(m_normal.at(0), m_normal.at(1), m_normal.at(2)); 
		}

		std::auto_ptr<NormalParameter> NormalParameter::clone() const
		{
			return std::auto_ptr<NormalParameter>(cloneImp());
		}

		NormalParameter::~NormalParameter()
		{
		}

		double NormalParameter::getX() const
		{
			return m_normal.at(0);
		}

		double NormalParameter::getY() const
		{
			return m_normal.at(1);
		}

		double NormalParameter::getZ() const
		{
			return m_normal.at(2);
		}
	}


}