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

#include "MantidVatesAPI/Common.h"
#include "MDDataObjects/MDImage.h"

namespace Mantid
{
  namespace VATES
  {
    template <typename Image>
    class DimensionComparitor
    {
    public:
      
    DimensionComparitor(boost::shared_ptr<Image> image): m_image(image)
    {
    }

    ~DimensionComparitor()
    {
    }

    bool isXDimension(Dimension_sptr queryDimension)
    {
      //Compare dimensions on the basis of their ids.
      Dimension_sptr actualXDimension = m_image->getGeometry()->getXDimension();
      return queryDimension->getDimensionId() == actualXDimension->getDimensionId();
    }

    bool isYDimension(Dimension_sptr queryDimension)
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

    bool isZDimension(Dimension_sptr queryDimension)
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

    bool istDimension(Dimension_sptr queryDimension)
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

    private:
      
      /// mdimage containing geometry.
      boost::shared_ptr<Image> m_image;
     
      DimensionComparitor operator=(DimensionComparitor&);
      
      DimensionComparitor(DimensionComparitor&);

    };

  }
}

#endif