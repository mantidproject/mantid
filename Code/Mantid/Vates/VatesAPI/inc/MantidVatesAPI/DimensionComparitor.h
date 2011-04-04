#ifndef MANTID_VATES_DIMENSION_COMPARITOR_H
#define MANTID_VATES_DIMENSION_COMPARITOR_H

/** Dimension comparitor specifically for use with visualisation layer. Given an arrangement of dimensions in an MDImage, this type
allow the utilising code to ask wheter some dimension maps to the x, y, or z dimensions.

 @author Owen Arnold, Tessella plc
 @date 25/03/2011

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Common.h"

namespace Mantid
{
  namespace VATES
  {
    class DimensionComparitor
    {
    public:
      
    DimensionComparitor(Mantid::API::IMDWorkspace_sptr workspace): m_workspace(workspace)
    {
    }

    ~DimensionComparitor()
    {
    }

    bool isXDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
    {
      //Compare dimensions on the basis of their ids.
      Mantid::Geometry::IMDDimension_const_sptr actualXDimension = m_workspace->getXDimension();
      return queryDimension->getDimensionId() == actualXDimension->getDimensionId();
    }

    bool isYDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
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

    bool isZDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
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

    bool istDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension)
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

    private:
      
      /// mdimage containing geometry.
      Mantid::API::IMDWorkspace_sptr m_workspace;
     
      DimensionComparitor operator=(DimensionComparitor&);
      
      DimensionComparitor(DimensionComparitor&);

    };

  }
}

#endif
