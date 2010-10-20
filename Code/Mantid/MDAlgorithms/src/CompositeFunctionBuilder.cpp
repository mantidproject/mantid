#include "MantidMDAlgorithms/CompositeFunctionBuilder.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

#include <exception>

namespace Mantid
{
	namespace MDAlgorithms
	{
		CompositeFunctionBuilder::CompositeFunctionBuilder()
		{
		}

		void CompositeFunctionBuilder::addFunctionBuilder(IFunctionBuilder* funcBuilder)
		{
			this->m_functionBuilders.push_back(boost::shared_ptr<IFunctionBuilder>(funcBuilder));
		}

		void CompositeFunctionBuilder::addParameter(IParameter& parameter)
		{
			std::string message = "PlaneFunctionBuilder does not take parameters of type: " + parameter.getName();
			throw std::invalid_argument(message);
		}
		
		std::auto_ptr<IImplicitFunction> CompositeFunctionBuilder::create() const
		{
			CompositeImplicitFunction* compFunction = new CompositeImplicitFunction;

			std::vector<boost::shared_ptr<IFunctionBuilder>>::const_iterator it;
			for(it = this->m_functionBuilders.begin(); it != this->m_functionBuilders.end(); ++it)
			{
				IImplicitFunction* rawFunc = (*it)->create().release();
				compFunction->addFunction(boost::shared_ptr<IImplicitFunction>(rawFunc));
			}

			return std::auto_ptr<IImplicitFunction>(compFunction);
		}

		CompositeFunctionBuilder::~CompositeFunctionBuilder()
		{
		}
	}


}