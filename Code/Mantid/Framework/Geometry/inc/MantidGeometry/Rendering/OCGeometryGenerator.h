#ifndef OC_GEOMETRYGENERATOR_H
#define OC_GEOMETRYGENERATOR_H

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <boost/shared_ptr.hpp>

class TopoDS_Shape;

namespace Mantid
{
  namespace Kernel
  {
    class Logger;
  }

	namespace Geometry
	{

		/**
		\class OCGeometryGenerator
		\brief Generates OpenCascade geometry from the ObjComponent
		\author Mr. Srikanth Nagella
		\date 4.08.2008
		\version 1.0

		This class is an OpenCascade geometry generation that takes in input as ObjComponent.

		Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
			///Analyze the object
			void AnalyzeObject();
			///Analyze a geometry rule for n object rule
			TopoDS_Shape AnalyzeRule(Rule*);
			///Analyze a geometry rule for an intersection
			TopoDS_Shape AnalyzeRule(Intersection*);
			///Analyze a geometry rule for a union
			TopoDS_Shape AnalyzeRule(Union*);
			///Analyze a geometry rule for a surface rule
			TopoDS_Shape AnalyzeRule(SurfPoint*);
			///Analyze a geometry rule for a Complement group
			TopoDS_Shape AnalyzeRule(CompGrp*);
			///Analyze a geometry rule for a complement object
			TopoDS_Shape AnalyzeRule(CompObj*);
						///Analyze a geometry rule for a boolean value
			TopoDS_Shape AnalyzeRule(BoolValue*);
						///create an OpenCascade shape from a Mantid shape
			TopoDS_Shape CreateShape(Surface* surf,int orientation);
						///create an OpenCascade sphere from a Mantid shape
			TopoDS_Shape CreateSphere(Sphere*);
						///create an OpenCascade cylinder from a Mantid shape
			TopoDS_Shape CreateCylinder(Cylinder*);
						///create an OpenCascade cone from a Mantid shape
			TopoDS_Shape CreateCone(Cone*);
						///create an OpenCascade plane from a Mantid shape
			TopoDS_Shape CreatePlane(Plane*,int orientation);
			///create an OpenCascade Torus from a Mantid shape
			TopoDS_Shape CreateTorus(Torus*);
		public:
			OCGeometryGenerator(const Object *obj);
			~OCGeometryGenerator();
			void Generate();
			TopoDS_Shape* getObjectSurface();
            /// return number of triangles in mesh (0 for special shapes)
			int getNumberOfTriangles();
            /// return number of points used in mesh (o for special shapes)
			int getNumberOfPoints();
            /// get a pointer to the 3x(NumberOfPoints) coordinates (x1,y1,z1,x2..) of mesh
			double* getTriangleVertices();
            /// get a pointer to the 3x(NumberOFaces) integers describing points forming faces (p1,p2,p3)(p4,p5,p6)..
			int* getTriangleFaces();
		};

	}    // NAMESPACE Geometry

}    // NAMESPACE Mantid

#endif
