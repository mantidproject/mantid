#include "MantidMDAlgorithms/BoxFunctionBuilder.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        using namespace Mantid::API;

        BoxFunctionBuilder::BoxFunctionBuilder()
        {
        }

        void BoxFunctionBuilder::addOriginParameter(const OriginParameter& originParam)
        {
          this->m_origin = originParam;
        }

        void BoxFunctionBuilder::addWidthParameter(const WidthParameter& widthParam)
        {
          this->m_width = widthParam;
        }

        void BoxFunctionBuilder::addHeightParameter(const HeightParameter& heightParam)
        {
          this->m_height = heightParam;
        }

        void BoxFunctionBuilder::addDepthParameter(const DepthParameter& depthParam)
        {
          this->m_depth = depthParam;
        }

        ImplicitFunction* BoxFunctionBuilder::create() const
        {
            //check that builder parameters are valid.
            if(!m_origin.isValid())
            {
                std::string message = "Invalid origin parameter on BoxFunctionBuilder";
                throw std::invalid_argument(message);
            } 
            if(!m_depth.isValid())
            {
                std::string message = "Invalid depth parameter passed to BoxFunctionBuilder";
                throw std::invalid_argument(message);
            }
            if(!m_width.isValid())
            {
              std::string message = "Invalid width parameter passed to BoxFunctionBuilder";
              throw std::invalid_argument(message);
            }
            if(!m_height.isValid())
            {
                std::string message = "Invalid height parameter passed to BoxFunctionBuilder";
                throw std::invalid_argument(message);
            }

            //implement construction.
            return new Mantid::MDAlgorithms::BoxImplicitFunction(m_width, m_height, m_depth, m_origin);
   
        }

        BoxFunctionBuilder::~BoxFunctionBuilder()
        {
        }
    }


}
