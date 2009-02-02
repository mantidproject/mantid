#ifndef CACHE_GEOMETRYGENERATOR_H
#define CACHE_GEOMETRYGENERATOR_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "boost/shared_ptr.hpp"

namespace Mantid
{

	namespace Geometry
	{

		/*!
		\class CacheGeometryGenerator
		\brief Generates geometry using other geometry handlers or keeps the cache of the triangles
		\author Mr. Srikanth Nagella
		\date Jan 2009
		\version 1.0

		This class is an cache for the geometry triangles and if needed generates the triangles using
		other GeometryHandlers.

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
		class DLLExport CacheGeometryGenerator
		{
		private:			
			static Kernel::Logger& PLog; ///< Static reference to the logger class
			Object *Obj; ///< Input Object
			int mNoOfVertices;
			int mNoOfTriangles;
			double* mPoints;
			int* mFaces;
		public:
			CacheGeometryGenerator(Object *obj);
			~CacheGeometryGenerator();
			void Generate();
			int getNumberOfTriangles();
			int getNumberOfPoints();
			double* getTriangleVertices();
			int* getTriangleFaces();
			void setGeometryCache(int noPts,int noFaces,double* pts,int* faces);
		};

	}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif
