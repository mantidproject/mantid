#include "MantidMDAlgorithms/DimensionParameterIntegration.h"
#include "MantidMDAlgorithms/DimensionParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{

  DimensionParameterIntegration::DimensionParameterIntegration(double upperLimit, double lowerLimit): m_upperLimit(upperLimit), m_lowerLimit(lowerLimit)
  {
  }

  inline double DimensionParameterIntegration::getUpperLimit() const
  {
    return this->m_upperLimit;
  }

  inline double DimensionParameterIntegration::getLowerLimit() const 
  {
    return this->m_lowerLimit;
  }

  bool DimensionParameterIntegration::isIntegrated() const
  {
    return true;
  }
}
}