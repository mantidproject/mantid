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

        PlaneFunctionBuilder::PlaneFunctionBuilder()
        {
        }

        void  PlaneFunctionBuilder::addNormalParameter(const NormalParameter& parameter)
        {
            
            this->m_normal = NormalParameter(parameter);
        }

        void  PlaneFunctionBuilder::addOriginParameter(const OriginParameter& parameter)
        { 
           
            this->m_origin = OriginParameter(parameter);
        }

        void PlaneFunctionBuilder::addWidthParameter(const WidthParameter& width)
        {
             this->m_width = WidthParameter(width);
        }

        ImplicitFunction* PlaneFunctionBuilder::create() const
        {
            //check that builder parameters are valid.
            if(!m_origin.isValid())
            {
                std::string message = "Invalid origin parameter on PlaneFunctionBuilder";
                throw std::invalid_argument(message);
            } 
            if(!m_normal.isValid())
            {
                std::string message = "Invalid normal parameter passed to PlaneFunctionBuilder";
                throw std::invalid_argument(message);
            }
            if(!m_width.isValid())
            {
              std::string message = "Invalid width parameter passed to PlaneFunctionBuilder";
              throw std::invalid_argument(message);
            }
            //implement construction.
            NormalParameter& refNormal = m_normal;
            OriginParameter& refOrigin = m_origin;
            WidthParameter& refWidth = m_width;
            PlaneImplicitFunction* func = new Mantid::MDAlgorithms::PlaneImplicitFunction(refNormal, refOrigin, refWidth);
            return func;
        }


        PlaneFunctionBuilder::~PlaneFunctionBuilder()
        {
        }
    }


}
