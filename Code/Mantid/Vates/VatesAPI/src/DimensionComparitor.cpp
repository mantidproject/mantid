#include "MantidVatesAPI/DimensionComparitor.h"

namespace Mantid
{
  namespace VATES
  {

    DimensionComparitor::DimensionComparitor(Mantid::API::IMDWorkspace_sptr workspace): m_workspace(workspace)
    {
    }

    DimensionComparitor::~DimensionComparitor()
    {
    }

    bool DimensionComparitor::isXDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
    {
      //Compare dimensions on the basis of their ids.
      Mantid::Geometry::IMDDimension_const_sptr actualXDimension = m_workspace->getXDimension();
      return queryDimension->getDimensionId() == actualXDimension->getDimensionId();
    }

    bool DimensionComparitor::isYDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
    {
      Mantid::Geometry::IMDDimension_const_sptr actualYDimension = m_workspace->getYDimension();
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

    bool DimensionComparitor::isZDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
    {
      Mantid::Geometry::IMDDimension_const_sptr actualZDimension = m_workspace->getZDimension();
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

    bool DimensionComparitor::istDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
    {
      Mantid::Geometry::IMDDimension_const_sptr actualtDimension = m_workspace->getTDimension();
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