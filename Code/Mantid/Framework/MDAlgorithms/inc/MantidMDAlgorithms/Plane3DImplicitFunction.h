#ifndef MANTID_ALGORITHMS_PLANE3DIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_PLANE3DIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <gsl/gsl_blas.h>
#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidKernel/Matrix.h"
#include "MantidMDAlgorithms/OriginParameter.h"
#include "MantidMDAlgorithms/NormalParameter.h"
#include "MantidMDAlgorithms/WidthParameter.h"
#include "MantidMDAlgorithms/UpParameter.h"
#include "MantidMDAlgorithms/PerpendicularParameter.h"
#include "MantidMDAlgorithms/VectorMathematics.h"
#include "MantidGeometry/MDGeometry/MDImplicitFunction.h"

namespace Mantid
{

    namespace MDAlgorithms
    {
        /**

        This class represents a plane implicit function defined in 3 dimensions,
        used for communicating and implementing an operation against a MDWorkspace.

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

        class DLLExport Plane3DImplicitFunction : public Mantid::Geometry::MDImplicitFunction
        {
        public:
          Plane3DImplicitFunction(NormalParameter& normal, OriginParameter& origin,  WidthParameter& width);
          ~Plane3DImplicitFunction();
          std::string getName() const;
          std::string toXMLString() const;
          double getOriginX() const;
          double getOriginY() const;
          double getOriginZ() const;
          double getNormalX() const;
          double getNormalY() const;
          double getNormalZ() const;
          double getWidth() const;
          bool operator==(const Plane3DImplicitFunction &other) const;
          bool operator!=(const Plane3DImplicitFunction &other) const;
          static std::string functionName()
          {
            return "Plane3DImplicitFunction";
          }

        private:

          /// Plane Origin
          OriginParameter m_origin;
          /// Plane Normal
          NormalParameter m_normal;
          /// Plane Width
          WidthParameter m_width;

          /// Calculate the width applied to the normal direction resolved into the specified axis.
          inline double calculateNormContributionAlongAxisComponent(const Mantid::Kernel::V3D& axis) const;
          /// Get the effective normal vector to use in calculation.
          inline NormalParameter calculateEffectiveNormal(const OriginParameter& forwardOrigin) const;

        };
    }
}


#endif
