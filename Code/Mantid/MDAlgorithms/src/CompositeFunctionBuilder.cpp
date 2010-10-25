#include <exception>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "MantidMDAlgorithms/CompositeFunctionBuilder.h"
#include "MantidMDAlgorithms/CompositeImplicitFunction.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        using namespace Mantid::API;

        CompositeFunctionBuilder::CompositeFunctionBuilder()
        {
        }

        void CompositeFunctionBuilder::addFunctionBuilder(IFunctionBuilder* funcBuilder)
        {
            this->m_functionBuilders.push_back(boost::shared_ptr<IFunctionBuilder>(funcBuilder));
        }

        std::auto_ptr<Mantid::API::IImplicitFunction> CompositeFunctionBuilder::create() const
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