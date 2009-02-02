#ifndef OC_GEOMETRYGENERATOR_H
#define OC_GEOMETRYGENERATOR_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "boost/shared_ptr.hpp"

class TopoDS_Shape;
namespace Mantid
{

	namespace Geometry
	{

		/*!
		\class OCGeometryGenerator
		\brief Generates OpenCascade geometry from the ObjComponent
		\author Mr. Srikanth Nagella
		\date 4.08.2008
		\version 1.0

		This class is an OpenCascade geometry generation that takes in input as ObjComponent.

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
		class Intersection;
		class Union;
		class SurfPoint;
		class CompGrp;
		class CompObj;
		class BoolValue;
		class Rule;
		class Surface;
		class Cylinder;
		class Sphere;
		class Cone;
		class Plane;
		class Torus;
		class DLLExport OCGeometryGenerator
		{
		private:			
			static Kernel::Logger& PLog; ///< Static reference to the logger class
			const Object *Obj; ///< Input Object
			TopoDS_Shape* ObjSurface; ///< Storage for the output surface
			int   iGridSize; ///< Grid size for sampling the object
			void AnalyzeObject();
			TopoDS_Shape AnalyzeRule(Rule*);
			TopoDS_Shape AnalyzeRule(Intersection*);
			TopoDS_Shape AnalyzeRule(Union*);
			TopoDS_Shape AnalyzeRule(SurfPoint*);
			TopoDS_Shape AnalyzeRule(CompGrp*);
			TopoDS_Shape AnalyzeRule(CompObj*);
			TopoDS_Shape AnalyzeRule(BoolValue*);
			TopoDS_Shape CreateShape(Surface* surf,int orientation);
			TopoDS_Shape CreateSphere(Sphere*);
			TopoDS_Shape CreateCylinder(Cylinder*);
			TopoDS_Shape CreateCone(Cone*);
			TopoDS_Shape CreatePlane(Plane*,int orientation);
			TopoDS_Shape CreateTorus(Torus*);
		public:
			OCGeometryGenerator(const Object *obj);
			~OCGeometryGenerator();
			void Generate();
			TopoDS_Shape* getObjectSurface();
			int getNumberOfTriangles();
			int getNumberOfPoints();
			double* getTriangleVertices();
			int* getTriangleFaces();
		};

	}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif
