#include "MantidMDAlgorithms/DimensionParameterNoIntegration.h"
#include "MantidMDAlgorithms/DimensionParameter.h"
#include <stdexcept>
namespace Mantid
{
namespace MDAlgorithms
{
  DimensionParameterNoIntegration::DimensionParameterNoIntegration() :  DimensionParameterIntegration(0, 0)
  {
  }


  double DimensionParameterNoIntegration::getUpperLimit() const
  {
    throw std::logic_error("No upper limit without integration");
  }

  double DimensionParameterNoIntegration::getLowerLimit() const 
  {
    throw std::logic_error("No lower limit without integration");
  }

  bool DimensionParameterNoIntegration::isIntegrated() const
  {
    return false;
  }

}
}
