#ifndef GTS_GEOMETRYGENERATOR_H
#define GTS_GEOMETRYGENERATOR_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{

	namespace Geometry
	{

		/*!
		\class GtsGeometryGenerator
		\brief Generates gts surface geometry from the ObjComponent
		\author Mr. Srikanth Nagella
		\date 4.08.2008
		\version 1.0

		This class is an GTS geometry generation that takes in input as ObjComponent.

		Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
		*/
		class Object;
		class DLLExport GtsGeometryGenerator
		{
		private:			
			static Kernel::Logger& PLog; ///< Static reference to the logger class
			const Object *Obj; ///< Input Object
			GtsSurface* ObjSurface; ///< Storage for the output surface
			int   iGridSize; ///< Grid size for sampling the object
		public:
			GtsGeometryGenerator(const Object *obj);
			~GtsGeometryGenerator();
			//virtual void init();
			void Generate();
			GtsSurface* getObjectSurface();
		};

	}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif
