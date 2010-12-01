#include "MantidMDAlgorithms/DimensionParameterSet.h"
#include "MantidMDAlgorithms/DimensionParameter.h"
#include <stdexcept>
namespace Mantid
{
namespace MDAlgorithms
{

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::findDimensionThrow(unsigned int id)
  {
    boost::shared_ptr<DimensionParameter> item = findDimension(id);
    if(NULL == item)
    {
      std::string message = std::string("Cannot find specified dimension id ");
      throw std::runtime_error(message);
    }
    return item;
  }

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::findDimension(unsigned int id)
  {
    boost::shared_ptr<DimensionParameter> item;
    std::vector<boost::shared_ptr<DimensionParameter> >::iterator it = m_dimensionParameters.begin();
    for( ; it != m_dimensionParameters.end(); ++it)
    {
      boost::shared_ptr<DimensionParameter> dimParam = (*it);
      if(dimParam->getId() == id)
      {
        item = dimParam;
        break;
      }
    }
    return item;
  }

  void DimensionParameterSet::addDimensionParameter(DimensionParameter* dParameter)
  {
    boost::shared_ptr<DimensionParameter> item = findDimension(dParameter->getId());
    if(NULL == item)
    {
      m_dimensionParameters.push_back(boost::shared_ptr<DimensionParameter>(dParameter));
    }
    else
    {
      throw std::logic_error("Cannot add the same dimension more than once.");
    }
  }

  std::vector<boost::shared_ptr<DimensionParameter> > DimensionParameterSet::getDimensions()
  {
    return m_dimensionParameters;
  }

  void DimensionParameterSet::setXDimension(unsigned int id)
  {
    this->m_dimX = findDimensionThrow(id);
  }

  void DimensionParameterSet::setYDimension(unsigned int id)
  {
    this->m_dimY = findDimensionThrow(id);
  }

  void DimensionParameterSet::setZDimension(unsigned int id)
  {
    this->m_dimZ = findDimensionThrow(id);
  }

  void DimensionParameterSet::settDimension(unsigned int id)
  {
    this->m_dimt = findDimensionThrow(id);
  }

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::getXDimension()
  {
    return this->m_dimX;
  }

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::getYDimension()
  {
    return this->m_dimY;
  }

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::getZDimension()
  {
    return this->m_dimZ;
  }

  boost::shared_ptr<DimensionParameter> DimensionParameterSet::gettDimension()
  {
    return this->m_dimt;
  }
}
}
