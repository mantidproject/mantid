#ifndef MANTID_ALGORITHMS_PLANEIMPLICITFUNCTION_H_
#define MANTID_ALGORITHMS_PLANEIMPLICITFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidKernel/System.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "IImplicitFunction.h"
#include "VectorMathematics.h"
using namespace Mantid::Kernel;
using namespace Mantid::API;

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
		class DLLExport PlaneImplicitFunction : IImplicitFunction
		{
		public:
			PlaneImplicitFunction::PlaneImplicitFunction(std::vector<double> normal, std::vector<double> origin);
			~PlaneImplicitFunction();
			bool Evaluate(MDDataObjects::point3D const * const pPoint) const;
			double GetOriginX() const;
			double GetOriginY() const;
			double GetOriginZ() const;
			double GetNormalX() const;
			double GetNormalY() const;
			double GetNormalZ() const;

		private:
			PlaneImplicitFunction(const PlaneImplicitFunction& other);
			PlaneImplicitFunction& operator=(const PlaneImplicitFunction& other);
			double originX;
			double originY;
			double originZ;
			double normalX;
			double normalY;
			double normalZ;

		};
	}
}


#endif