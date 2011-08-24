#include <exception>
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

        void CompositeFunctionBuilder::addFunctionBuilder(ImplicitFunctionBuilder* funcBuilder)
        {
            this->m_functionBuilders.push_back(boost::shared_ptr<ImplicitFunctionBuilder>(funcBuilder));
        }

        Mantid::Geometry::MDImplicitFunction* CompositeFunctionBuilder::create() const
        {
            CompositeImplicitFunction* compFunction = new CompositeImplicitFunction;

            std::vector<boost::shared_ptr<ImplicitFunctionBuilder> >::const_iterator it;
            for(it = this->m_functionBuilders.begin(); it != this->m_functionBuilders.end(); ++it)
            {
                Mantid::Geometry::MDImplicitFunction* rawFunc = (*it)->create();
                compFunction->addFunction(Mantid::Geometry::MDImplicitFunction_sptr(rawFunc));
            }

            return compFunction;
        }

        CompositeFunctionBuilder::~CompositeFunctionBuilder()
        {
        }
    }


}
