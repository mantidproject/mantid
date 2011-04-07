#ifndef MANTID_VATES_DIMENSION_COMPARITOR_H
#define MANTID_VATES_DIMENSION_COMPARITOR_H

#include "MantidAPI/IMDWorkspace.h"
#include "MantidVatesAPI/Common.h"

namespace Mantid
{
  namespace VATES
  {
    
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
    class DimensionComparitor
    {
    public:
     
    /// Constructor
    DimensionComparitor(Mantid::API::IMDWorkspace_sptr workspace);

    /// Destructor
    ~DimensionComparitor();

    bool isXDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension);
    
    bool isYDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension);

    bool isZDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension);

    bool istDimension(Mantid::Geometry::IMDDimension_const_sptr  queryDimension);

    private:
      
      /// imd workspace shared ptr.
      Mantid::API::IMDWorkspace_sptr m_workspace;
     
      DimensionComparitor operator=(DimensionComparitor&);
      
      DimensionComparitor(DimensionComparitor&);

    };

  }
}

#endif
