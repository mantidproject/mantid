#ifndef BOX_FUNCTONBUILDER_H_
#define BOX_FUNCTONBUILDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <memory>
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BoxImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionBuilder.h"

namespace Mantid
{

    namespace MDAlgorithms
    {
        /**

        This class is the abstract type for building BoxImplicitFunctions

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

        class DLLExport BoxFunctionBuilder : public Mantid::API::ImplicitFunctionBuilder
        {
        private:
            mutable OriginParameter m_origin; ///< Origin of the box
            mutable WidthParameter m_width; ///< Size for the box width
            mutable HeightParameter m_height; ///< Size for the box height
            mutable DepthParameter m_depth; ///< Size for the box depth
        public:
            BoxFunctionBuilder();

            /// Set the box origin
            void addOriginParameter(const OriginParameter& parameter);
            /// Set the box depth
            void addDepthParameter(const DepthParameter& depthParam);
            /// Set the box width
            void addWidthParameter(const WidthParameter& widthParam);
            /// Set the box height
            void addHeightParameter(const HeightParameter& heightParam);
            /// Create an implicit function instance
            Mantid::Geometry::MDImplicitFunction* create() const;
            ~BoxFunctionBuilder();
        };

    }
}

#endif
