#ifndef MANTID_MDALGORITHMS_BOX3DIMPLICITFUNCTION_H_
#define MANTID_MDALGORITHMS_BOX3DIMPLICITFUNCTION_H_
    
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidMDAlgorithms/DepthParameter.h"
#include "MantidMDAlgorithms/HeightParameter.h"
#include "MantidMDAlgorithms/OriginParameter.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"

namespace Mantid
{
    namespace MDAlgorithms
    {
        /**
        This class represents a box for making slices orthogonal to the cartesian axes set.
        This is a specialization of the MDBoxImplicitFunction for 3 dimensions.

        @author Owen Arnold, Tessella plc
        @date 09/12/2010

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

        class DLLExport BoxImplicitFunction : public Mantid::Geometry::MDBoxImplicitFunction
        {
        public:
            /**
             * Object constructor
             * @param width the value for the width of the box
             * @param height the value for the height of the box
             * @param depth the value for the depth of the box
             * @param origin the value for the origin of the box
             */
            BoxImplicitFunction(WidthParameter& width, HeightParameter& height, DepthParameter& depth, OriginParameter& origin);
            /// Object destructor
            ~BoxImplicitFunction();
            std::string getName() const;
            std::string toXMLString() const;
            /// Return the maximum extent of the x direction
            double getUpperX() const;
            /// Return the mimimum extent of the x direction
            double getLowerX() const;
            /// Return the maximum extent of the y direction
            double getUpperY() const;
            /// Return the mimimum extent of the y direction
            double getLowerY() const;
            /// Return the maximum extent of the z direction
            double getUpperZ() const;
            /// Return the mimimum extent of the z direction
            double getLowerZ() const;
            /**
             * Equality operator overload
             * @param other the object to test against
             * @return true if objects are equal
             */
            bool operator==(const BoxImplicitFunction &other) const;
            /**
             * Non-equality operator overload
             * @param other the object to test against
             * @return true if objects are not equal
             */
            bool operator!=(const BoxImplicitFunction &other) const;

            /// Return the function name
            static std::string functionName()
            {
                return "BoxImplicitFunction";
            }

        private:
            //from raw inputs.
            OriginParameter m_origin; ///< Origin of box
            HeightParameter m_height; ///< Height of box
            WidthParameter m_width; ///< Width of box
            DepthParameter m_depth; ///< Depth of box

            // Bounds of the box
            std::vector<coord_t> min; ///< Minimum extents on all axes
            std::vector<coord_t> max; ///< Maximum extents on all axes

        };
    }
}


#endif  /* MANTID_MDALGORITHMS_BOX3DIMPLICITFUNCTION_H_ */
