#include "MantidMDAlgorithms/CompositeImplicitFunction.h"
#include "MDDataObjects/point3D.h"

namespace Mantid
{
	namespace MDAlgorithms
	{

		CompositeImplicitFunction::CompositeImplicitFunction()
		{	
		}

		CompositeImplicitFunction::~CompositeImplicitFunction()
		{
		}

		void CompositeImplicitFunction::AddFunction(boost::shared_ptr<IImplicitFunction> constituentFunction)
		{
			this->m_Functions.push_back(constituentFunction);
		}

		bool CompositeImplicitFunction::Evaluate(MDDataObjects::point3D const * const pPoint3D) const
		{
			bool evalResult = false;
			std::vector<boost::shared_ptr<Mantid::API::IImplicitFunction>>::const_iterator it;
			for(it = this->m_Functions.begin(); it != this->m_Functions.end(); ++it)
			{
				evalResult = (*it)->Evaluate(pPoint3D);
				if(!evalResult)
				{
					break;
				}
			}
			return evalResult;
		}


	}
}