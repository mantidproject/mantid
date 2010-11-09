#include "MantidMDAlgorithms/DimensionParameter.h"
#include "MantidMDAlgorithms/DimensionParameterIntegration.h"
#include <stdexcept>
namespace Mantid
{
namespace MDAlgorithms
{
  DimensionParameter::DimensionParameter(unsigned int id, std::string name, double upperBounds, double lowerBounds, boost::shared_ptr<DimensionParameterIntegration> integration) : m_id(id), m_name(name), m_upperBounds(upperBounds), m_lowerBounds(lowerBounds)
  {
    if(upperBounds < lowerBounds)
    {
      throw std::logic_error("Upper bounds must be > lower bounds for the dimension.");
    }

    if(integration->isIntegrated()) 
    {
      if(integration->getLowerLimit() < this->m_lowerBounds)
      {
        throw std::out_of_range("Lower limit of integration cannot be below the lower bounds of the dimension.");
      }
      if(integration->getUpperLimit() > this->m_upperBounds)
      {
        throw std::out_of_range("Upper limit of integration cannot be above the upper bounds of the dimension.");
      }
    }

    m_integration = integration;
  }

  boost::shared_ptr<DimensionParameterIntegration> DimensionParameter::getIntegration() const
  {
    return this->m_integration;
  }

  void DimensionParameter::setIntegration(DimensionParameterIntegration* integration)
  {
    this->m_integration.reset(integration);
  }

  double DimensionParameter::getUpperBound() const
  {
    return this->m_upperBounds;
  }

  double DimensionParameter::getLowerBound() const
  {
    return this->m_lowerBounds;
  }

  std::string DimensionParameter::getName() const
  {
    return this->m_name;
  }

  unsigned int DimensionParameter::getId() const
  {
    return this->m_id;
  }

}
}
