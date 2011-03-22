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
            mutable OriginParameter m_origin;
            mutable WidthParameter m_width;
            mutable HeightParameter m_height;
            mutable DepthParameter m_depth;
        public:
            BoxFunctionBuilder();

            void addOriginParameter(const OriginParameter& parameter);
            void addDepthParameter(const DepthParameter& depthParam);
            void addWidthParameter(const WidthParameter& widthParam);
            void addHeightParameter(const HeightParameter& heightParam);
            Mantid::API::ImplicitFunction* create() const;
            ~BoxFunctionBuilder();
        };

    }
}

#endif
