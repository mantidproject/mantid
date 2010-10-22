#ifndef MANTID_ALGORITHMS_PLANEIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_PLANEIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "MantidMDAlgorithms/IImplicitFunction.h"
#include "MantidMDAlgorithms/OriginParameter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include "MantidMDAlgorithms/VectorMathematics.h"

namespace Mantid
{
    namespace MDDataObjects
    {
        class point3D;
    }
    namespace MDAlgorithms
    {
        /** A base class for absorption correction algorithms.


        This class represents a plane implicit function used for communicating and implementing an operation against 
        an MDWorkspace.

        @author Owen Arnold, Tessella plc
        @date 01/10/2010

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

        //TODO. This should be constructed via a factory
        class DLLExport PlaneImplicitFunction : public Mantid::API::IImplicitFunction
        {
        public:
            PlaneImplicitFunction(NormalParameter normal, OriginParameter origin);
            ~PlaneImplicitFunction();
            bool evaluate(MDDataObjects::point3D const * const pPoint) const;
            double getOriginX() const;
            double getOriginY() const;
            double getOriginZ() const;
            double getNormalX() const;
            double getNormalY() const;
            double getNormalZ() const;

            static std::string functionName()
            {
                return "PlaneImplicitFunction";
            }

        private:
            PlaneImplicitFunction(const PlaneImplicitFunction& other);
            PlaneImplicitFunction& operator=(const PlaneImplicitFunction& other);
            OriginParameter m_origin;
            NormalParameter m_normal;

        };
    }
}


#endif