#ifndef MANTID_ALGORITHMS_BOXIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_BOXIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidMDAlgorithms/DepthParameter.h"
#include "MantidMDAlgorithms/HeightParameter.h"
#include "MantidMDAlgorithms/OriginParameter.h" 

namespace Mantid
{
    namespace MDDataObjects
    {
        class point3D;
    }
    namespace MDAlgorithms
    {
        /**

        This class represents a box for making slices orthogonal to the cartesian axes set.

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

        class DLLExport BoxImplicitFunction : public Mantid::API::ImplicitFunction
        {
        public:
            BoxImplicitFunction(WidthParameter& width, HeightParameter& height, DepthParameter& depth, OriginParameter& origin);
            ~BoxImplicitFunction();
            std::string getName() const;
            std::string toXMLString() const;
            bool evaluate(const API::Point3D* pPoint) const;
            double getUpperX() const;
            double getLowerX() const;
            double getUpperY() const;
            double getLowerY() const;
            double getUpperZ() const;
            double getLowerZ() const;
            bool operator==(const BoxImplicitFunction &other) const;
            bool operator!=(const BoxImplicitFunction &other) const;

            static std::string functionName()
            {
                return "BoxImplicitFunction";
            }

        private:

            //from raw inputs.
            OriginParameter m_origin;
            HeightParameter m_height;
            WidthParameter m_width;
            DepthParameter m_depth;

            //calculated and cached.
            double m_upperX;
            double m_lowerX;
            double m_upperY;
            double m_lowerY;
            double m_upperZ;
            double m_lowerZ;

        };
    }
}


#endif
