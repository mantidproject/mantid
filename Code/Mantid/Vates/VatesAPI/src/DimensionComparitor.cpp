#include "MantidVatesAPI/DimensionComparitor.h"

namespace Mantid
{
  namespace VATES
  {
    DimensionComparitor::DimensionComparitor(Mantid::MDDataObjects::MDImage_sptr image): m_image(image)
    {
    }

    DimensionComparitor::~DimensionComparitor()
    {
    }

    bool DimensionComparitor::isXDimension(Dimension_sptr queryDimension)
    {
      //Compare dimensions on the basis of their ids.
      Dimension_sptr actualXDimension = m_image->getGeometry()->getXDimension();
      return queryDimension->getDimensionId() == actualXDimension->getDimensionId();
    }

    bool DimensionComparitor::isYDimension(Dimension_sptr queryDimension)
    {
      Dimension_sptr actualYDimension = m_image->getGeometry()->getYDimension();
      if(NULL == actualYDimension.get())
      {
        return false; //MDImages may have 1 dimension or more.
      }
      else
      {
        //Compare dimensions on the basis of their ids.
        return queryDimension->getDimensionId() == actualYDimension->getDimensionId();
      }
    }

    bool DimensionComparitor::isZDimension(Dimension_sptr queryDimension)
    {
      Dimension_sptr actualZDimension = m_image->getGeometry()->getZDimension();
      if(NULL == actualZDimension.get())
      {
        return false; //MDImages may have 1 dimension or more.
      }
      else
      {
        //Compare dimensions on the basis of their ids.
        return queryDimension->getDimensionId() == actualZDimension->getDimensionId();
      }
    }

    bool DimensionComparitor::istDimension(Dimension_sptr queryDimension)
    {
      Dimension_sptr actualtDimension = m_image->getGeometry()->getTDimension();
      if(NULL == actualtDimension.get())
      {
        return false; //MDImages may have 1 dimension or more.
      }
      else
      {
        //Compare dimensions on the basis of their ids.
        return queryDimension->getDimensionId() == actualtDimension->getDimensionId();
      }
    }
  }
}