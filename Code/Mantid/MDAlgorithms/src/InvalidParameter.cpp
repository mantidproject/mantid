
#include "MantidMDAlgorithms/InvalidParameter.h"

namespace Mantid
{
	namespace MDAlgorithms
	{
		InvalidParameter::InvalidParameter() 
		{
		}
		
		std::string InvalidParameter::getName() const
		{
			return parameterName();
		}

		bool InvalidParameter::isValid() const
		{
			return false;
		}

		InvalidParameter* InvalidParameter::cloneImp() const
		{
			return new InvalidParameter;
		}

		std::auto_ptr<InvalidParameter> InvalidParameter::clone() const
		{
			return std::auto_ptr<InvalidParameter>(cloneImp());
		}

		InvalidParameter::~InvalidParameter()
		{
		}
	}


}