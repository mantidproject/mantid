#include "MantidMDAlgorithms/PlaneFunctionBuilder.h"
#include "MantidMDAlgorithms/InvalidParameter.h"
#include "MantidMDAlgorithms/OriginParameter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include <exception>

namespace Mantid
{
    namespace MDAlgorithms
    {
        using namespace Mantid::API;

        PlaneFunctionBuilder::PlaneFunctionBuilder(): origin(std::auto_ptr<IParameter>(new InvalidParameter())) , normal(std::auto_ptr<IParameter>(new InvalidParameter()))
        {
        }

        void PlaneFunctionBuilder::addParameter(std::auto_ptr<IParameter> parameter)
        {
            if(parameter->getName() == OriginParameter::parameterName())
            {
                this->origin = parameter->clone();
            }
            else if(parameter->getName() == NormalParameter::parameterName())
            {
                this->normal = parameter->clone();
            }
            else
            {
                std::string message = "PlaneFunctionBuilder does not take parameters of type: " + parameter->getName();
                throw std::invalid_argument(message);
            }
        }

        std::auto_ptr<IImplicitFunction> PlaneFunctionBuilder::create() const
        {
            if(!origin->isValid())
            {
                std::string message = "Invalid parameter passed to PlaneFunctionBuilder: " + origin->getName();
                throw std::invalid_argument(message);
            }
            if(!normal->isValid())
            {
                std::string message = "Invalid parameter passed to PlaneFunctionBuilder: " + normal->getName();
                throw std::invalid_argument(message);
            }

            OriginParameter originParam = *dynamic_cast<OriginParameter*>(origin.get());
            NormalParameter normalParam = *dynamic_cast<NormalParameter*>(normal.get());

            return std::auto_ptr<IImplicitFunction>(new PlaneImplicitFunction(normalParam, originParam));
        }

        PlaneFunctionBuilder::~PlaneFunctionBuilder()
        {
        }
    }


}