#include "CompositeImplicitFunction.h"
#include "Hexahedron.h"

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

		void CompositeImplicitFunction::AddFunction(IImplicitFunction* constituentFunction)
		{
			this->m_Functions.push_back(boost::shared_ptr<IImplicitFunction>(constituentFunction));
		}

		bool CompositeImplicitFunction::Evaluate(MDDataObjects::Hexahedron* pHexahedron)
		{
			bool evalResult = false;
			std::vector<boost::shared_ptr<IImplicitFunction>>::iterator it;
			for(it = this->m_Functions.begin(); it != this->m_Functions.end(); ++it)
			{
				evalResult = (*it)->Evaluate(pHexahedron);
				if(!evalResult)
				{
					break;
				}
			}
			return evalResult;
		}


	}
}